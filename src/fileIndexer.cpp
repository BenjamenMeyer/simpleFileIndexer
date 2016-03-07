#include <fileIndexer.h>

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <locale>
#include <map>

#include <QtGlobal>
#include <qtconcurrentmap.h>

#include <QDebug>
#include <QMultiMap>
#include <QTimer>

static FileIndexer* instance = NULL;

// Message Handler for qDebug() functionality to redirect to the logging functionality
void messageCapture(QtMsgType _type, const char* _msg)
	{
	// when received, send to the instance if it has been instantiated
	QString msg(_msg);
	if (instance != NULL)
		{
		instance->incomingMessage(_type, msg);
		}
	}

// Message to make it simple ot send data to the log from the functions used for the Map Reduce algorithm
void resultDebugLog(QString _fileName, QString _message)
	{
	qDebug() << "File Name: " << _fileName << " - " << _message;
	}

// C++ locale that only allows A-Z, a-z, and 0-9 characters
struct alphanumeric : std::ctype<char>
	{
	alphanumeric() : std::ctype<char>(table_alphanumeric()) {}

	static std::ctype_base::mask const* table_alphanumeric()
		{
		// start with an empty locale table
		static std::vector<std::ctype_base::mask> localTable(std::ctype<char>::table_size, std::ctype_base::space);
		// upper case A-Z
		std::fill(&localTable['A'], &localTable['Z'], std::ctype_base::upper|std::ctype_base::alpha);
		// lower case a-z
		std::fill(&localTable['a'], &localTable['z'], std::ctype_base::lower|std::ctype_base::alpha);
		// digits
		std::fill(&localTable['0'], &localTable['9'], std::ctype_base::digit);
		return &localTable[0];
		}
	};

// convenience function for accumulating the words for a given file
void addWord(WordCount& _results, QString wordToAdd, uint64_t _count=1)
	{
	// we want case insensitive comparison so unify the word to a single case
	QString actualWord = wordToAdd.toLower();

	// if it exists, increase it, otherwise add it initialized to the specified count
	if (_results.contains(actualWord) == true)
		{
		_results[actualWord] += _count;
		}
	else
		{
		_results.insert(actualWord, _count);
		}
	}

// index a single file
WordCount indexFile(QString fileName)
	{
	// results for the single file
	WordCount results;

	resultDebugLog(fileName, QString("Received file for processing"));

	std::ifstream inputData;
	// use the locale to discard any characters we do not want
	// this has the effect of easily allowing us to read one word from the file at a time,
	// and saving memory/cpu/resources in the parsing
	inputData.imbue(std::locale(std::locale(), new alphanumeric()));
	// open the file
	inputData.open(fileName.toLatin1().data(), std::ifstream::in);
	if (inputData.is_open() == true)
		{
		// word read in
		std::string rawWord;

		// add words so long as we can read one
		while (inputData >> rawWord)
			{
			// increase the count by 1 for each word read
			addWord(results, QString::fromStdString(rawWord));
			}

		// close the file
		inputData.close();

		qDebug() << "Completed processing " << fileName;
		}
	else
		{
		// the file could not be opened (permissions?) - record it so the user knows we found it
		// but were unable to open it for processing
		qDebug() << "Failed to open file \"" << fileName << "\"";
		}

	return results;
	}

// map reduce accumulator
void indexFileReducer(WordCount& _results, const WordCount& fileResult)
	{
	// Note: There is no guarantee which file will give its results back first as this can be completely asynchronous
	//       to the file list being process.

	// add the per-file results to the final results
	for (WordCount::const_iterator iter = fileResult.constBegin(); iter != fileResult.constEnd(); ++iter)
		{
		// increase the per-word count by the file's count
		addWord(_results, iter.key(), iter.value());
		}
	}

FileIndexer::FileIndexer(QStringList filesToAnalyze, QObject* _parent) : QObject(_parent), fileList(filesToAnalyze)
	{
	// capture log messages sent to qDebug()
	instance = this;
	qInstallMsgHandler(messageCapture);

	// capture log messages sent via a SIGNAL
	connect(this, SIGNAL(logMessage(QString)), &theLog, SLOT(message(QString)));

	// close the log before the application closes but after the log thread's event loop
	// has had the chance to process all messages sent to it
	connect(this, SIGNAL(close()), &theLog, SLOT(closeApplication())); 

	// setup the log file output and push logging to its own thread
	theLog.setLogFile(".fileIndexer.log");
	theLog.moveToThread(&logThread);
	logThread.start();

	// start processing once the event loop starts
	QTimer::singleShot(0, this, SLOT(runIndexer()));
	}

FileIndexer::~FileIndexer()
	{
	// wait for the log thread to terminate before allowing the object to be destroyed
	// this guarantees the log file will have all its data
	logThread.quit();
	logThread.wait();

	// clear the instance object just in case another one is made in after this one terminates
	// note: it cannot be reset until after the log closes in case there are outstanding log
	// messages that need to be captured via qDebug().
	instance = NULL;
	}

void FileIndexer::incomingMessage(QtMsgType _type, QString _msg)
	{
	// qDebug() provides some information regarding the type of log message being written
	// convert it to a nice little string since the log object in use doesn't use the log
	// types.
	switch (_type)
		{
		case QtDebugMsg:	_msg.prepend("Debug   : ");	break;
		case QtWarningMsg:	_msg.prepend("Warning : ");	break;
		case QtCriticalMsg:	_msg.prepend("Critical: ");	break;
		case QtFatalMsg:	_msg.prepend("Fatal   : ");	break;
		default:			_msg.prepend("Unknown : ");	break;
		};

	// now send it off to the log
	Q_EMIT logMessage(_msg);
	}

void FileIndexer::runIndexer()
	{
	Q_EMIT logMessage(tr("Starting File Indexing"));
	
	// only run if we have files to process
	if (fileList.size() > 0)
		{
		// use the Map Reduce algorithm to find the 10 top words
		anticipatedResults = QtConcurrent::mappedReduced(fileList, indexFile, indexFileReducer);

		// process the final results
		finalizeResults();
		}
	else
		{
		// no files to process. tell the user
		std::cerr << "NO files to index" << std::endl;
		Q_EMIT logMessage(tr("NO files to index"));
		}

	// everything is done, signal that it's time to close so the logs can clean up and terminate the program
	Q_EMIT close();
	}

void FileIndexer::finalizeResults()
	{
	Q_EMIT logMessage(tr("Waiting for indexing"));
	// Get the results, this may block
	WordCount results = anticipatedResults.result();
	Q_EMIT logMessage(tr("Finished indexing"));
	Q_EMIT logMessage(tr("Found %1 words").arg(results.size()));

	// now flip the results map so that we can capture the top 10 words
	Q_EMIT logMessage(tr("Building count-oriented listing"));
	typedef std::pair<uint64_t, QString> reverseWordCounterPair;
	typedef std::multimap<uint64_t, QString> reverseWordCounterMap;
	reverseWordCounterMap reverseMap;
	for (WordCount::const_iterator iter = results.constBegin(); iter != results.constEnd(); ++iter)
		{
		// flip the key and the value for the multimap
		reverseWordCounterPair newSet;
		newSet.first = iter.value();
		newSet.second = iter.key();
		reverseMap.insert(newSet);
		}

	// output the final results to stdout and to the log
	// Note: std::multimap keeps things ordered for us. So the top 10 items are at the back of the map.
	//		A simple reverse iterator can be used to capture them without further lookups
	// output the 10 top used words by looking at the back 10 items of the map
	Q_EMIT logMessage(tr("Generating Top-10 List"));
	uint32_t count = 0;
	std::cout<<"Top 10 Words:"<<std::endl;
	Q_EMIT logMessage(tr("Top 10 Words:"));
	for (reverseWordCounterMap::reverse_iterator rIter = reverseMap.rbegin(); rIter != reverseMap.rend(); ++rIter)
		{
		std::cout<<"\t"<<rIter->second.toLatin1().data()<<" - "<<rIter->first<<" times."<<std::endl;
		Q_EMIT logMessage(tr("%1 - %2 times").arg(rIter->second).arg(rIter->first));
		++count;
		if (count >= 10) break;
		}
	
	// warn if we didn't find enough words to make 10
	if (count < 10)
		{
		std::cout<<std::endl<<"Only "<<count<<" words were found in the files."<<std::endl;
		Q_EMIT logMessage(tr("Only %1 words were found in the file.").arg(count));
		}

	// add a blank line
	std::cout<<std::endl;
	}
