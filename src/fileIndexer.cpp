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

/*! \brief Log Capture Interface
 *
 *  Message Handler for qDebug() functionality to redirect to the logging functionality
 *
 *  \param _type - Message Type (f.e Debug, Info, etc)
 *  \param _msg - Log Message
 */
void messageCapture(QtMsgType _type, const char* _msg)
    {
    // when received, send to the instance if it has been instantiated
    QString msg(_msg);
    if (instance != NULL)
        {
        instance->incomingMessage(_type, msg);
        }
    }

/*! \brief File Processing Logging
 *
 *  Convenience method for logging messages for a given file being processed
 *
 *  \param _filename - the filename being processed
 *  \param _message - the log message to be recorded
 */
void resultDebugLog(QString _fileName, QString _message)
    {
    qDebug() << "File Name: " << _fileName << " - " << _message;
    }

//! C++ locale that only allows A-Z, a-z, and 0-9 characters as the definition of a "word"
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

/*! \brief Increase the word count in the result
 *
 *  Convenience method for increading the count for a given word in the result
 *
 *  \param _results - result object to increase the count in
 *  \param wordToAdd - word the count is for
 *  \param _count - the count to increment by
 */
void addWord(WordCount& _results, QString wordToAdd, uint64_t _count=1)
    {
    // case insensitive comparison to unify the word to a single case
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

/*! \brief Single File Word Indexing
 *
 *  Count the words in a given file
 *
 *  \param fileName - filename to process
 *
 *  \return WordCount object containing the counts of all words in the file
 */
WordCount indexFile(QString fileName)
    {
    // results for the single file
    WordCount results;

    resultDebugLog(fileName, QString("Received file for processing"));

    std::ifstream inputData;
    // use the locale to discard any undesired characters
    // this has the effect of easily allowing one "word" to be read from the file at a time,
    // and saving memory/cpu/resources in the parsing
    inputData.imbue(std::locale(std::locale(), new alphanumeric()));

    // open the file
    inputData.open(fileName.toLatin1().data(), std::ifstream::in);
    if (inputData.is_open() == true)
        {
        // word read in
        std::string rawWord;

        // add words so long as one can be read in
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
        // the file could not be opened (permissions?) - record it so the user knows it was found
        // but was unable to open it for processing
        qDebug() << "Failed to open file \"" << fileName << "\"";
        }

    return results;
    }

/*! \brief Word Count MapReduce Accumulator
 *
 *  MapReduce splits out the processing between multiple workers. The accumulator combines the results
 *  back into a single result
 *
 *  \param _results - the final result object
 *  \param fileResult - the individual file results
 */
void indexFileReducer(WordCount& _results, const WordCount& fileResult)
    {
    // Note: There is no guarantee which file will give its results back first as this is completely asynchronous
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
        case QtDebugMsg:    _msg.prepend("Debug   : ");    break;
        case QtWarningMsg:  _msg.prepend("Warning : ");    break;
        case QtCriticalMsg: _msg.prepend("Critical: ");    break;
        case QtFatalMsg:    _msg.prepend("Fatal   : ");    break;
        default:            _msg.prepend("Unknown : ");    break;
        };

    // now send it off to the log
    Q_EMIT logMessage(_msg);
    }

void FileIndexer::runIndexer()
    {
    Q_EMIT logMessage(tr("Starting File Indexing"));

    // only run if there are files to process
    if (fileList.size() > 0)
        {
        // use the Map Reduce algorithm to count all the words in the specified files
        anticipatedResults = QtConcurrent::mappedReduced(fileList, indexFile, indexFileReducer);

        // process the results to capture the top 10 words
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

    // the results contain the counts for all words in all files in
    // a mapping of word to count. However, the requirement is only for
    // the 10 top words across all the files being processed
    // therefore, flip the mapping from word:count to count:word,
    // a multimap is used to allow duplicate entries
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
    // Note: Since the 'key' of the multimap is the numeric count, std::multimap will keep everything ordered.
    //       As a result, the top 10 words are at the end of the key list, and a simple reverse iterator can be
    //       used to capture them without further lookups.

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

    // warn if there were not enough words to make 10
    if (count < 10)
        {
        std::cout<<std::endl<<"Only "<<count<<" words were found in the files."<<std::endl;
        Q_EMIT logMessage(tr("Only %1 words were found in the file.").arg(count));
        }

    // add a blank line
    std::cout<<std::endl;
    }
