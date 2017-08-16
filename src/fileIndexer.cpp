#include <fileIndexer.h>

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <locale>
#include <map>
#include <strings.h>

#include <QtGlobal>
#include <qtconcurrentmap.h>

#include <QDebug>
#include <QFile>
#include <QMultiMap>
#include <QRegExp>
#include <QTimer>

//! instance pointer used for capturing log data
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

void resultDebugLog(QString _fileName, QString _message)
    {
    qDebug() << "File Name: " << _fileName << " - " << _message;
    }

void addWord(WordCount& _results, QString wordToAdd, uint64_t _count)
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

WordCount indexFile(QString fileName)
    {
    // results for the single file
    WordCount results;

    // log which file is being processed
    resultDebugLog(fileName, QString("Received file for processing"));

    QFile inputData(fileName);
    // buffer information
    const unsigned int MAX_INPUT_BUFFER = 32768;
    const unsigned int MAX_READ = MAX_INPUT_BUFFER - 1;

    // attempt to open the file
    if (inputData.open(QIODevice::ReadOnly|QIODevice::Text) == true)
        {
        // processing buffer
        QString totalBuffer;

        // read buffer
        char buffer[MAX_INPUT_BUFFER];

        // whether or not to consider a word that reaches the end of the
        // buffer a word. If true, consider it a word and terminate; if
        // false add more data and reprocess
        bool empty_buffer = false;
        do
            {
            // prevent data leakage
            bzero(buffer, MAX_INPUT_BUFFER);

            // always read one less than the buffer size, ensuring
            // a zero termination string
            qint64 dataRead = inputData.read(buffer, MAX_READ);

            resultDebugLog(fileName, QString("Read %1 additional bytes").arg(dataRead));

            // -1 -> error, 0 = EOF
            empty_buffer = (dataRead <= 0);

            // add the new data to the buffer
            totalBuffer += QString::fromLatin1(buffer);

            // count all words in the buffer
            processBuffer(fileName, totalBuffer, empty_buffer, results);

            resultDebugLog(fileName, QString("Remaining buffer size: %1 bytes").arg(totalBuffer.length()));

            // continue so long as there is data in the file
            } while (!empty_buffer);
        }
    else
        {
        // error reading the file - nothing will be counted from it
        resultDebugLog(fileName, QString("Unable to open file - no counts added"));
        }

    // send the results back to MapReduce
    return results;
    }

void processBuffer(const QString& fileName, QString& buffer, bool allow_ending_word, WordCount& results)
    {
    // regular expression matching
    QRegExp wordMatcher(
        "[A-Za-z0-9]+",            // pattern - AZa-z0-9; letters with accents won't be counted
        Qt::CaseInsensitive,    // case sensitivity
        QRegExp::RegExp2        // pattern syntax type (Qt5 default, PERL like-greedy)
    );
    // now process the buffer
    bool process_buffer = true;
    while (process_buffer)
        {
        // test the buffer to see if any matches occur
        int matchIndex = wordMatcher.indexIn(buffer);
        if ( matchIndex == -1)
            {
            // note: this means there are zero remaining matches in the buffer
            //    thus the entire buffer can be tossed
            resultDebugLog(fileName, QString("No more matches - clearing buffer"));
            buffer.clear();

            // terminate the loop
            process_buffer = false;
            }
        else
            {
            // grab the single word from the buffer
            QString wordRead = wordMatcher.cap(0);

            // check if the word goes to the end of the buffer
            if ((wordRead.length() + matchIndex) == buffer.length())
                {
                // at the end of the buffer
                // does the word count as a word, or is more data needed
                if (!allow_ending_word)
                    {
                    // more data is required...
                    process_buffer = false;
                    break;
                    }
                }

            // increase the count by 1 for each word read
            addWord(results, wordRead);

            // remove the word from the buffer
            buffer.remove(0, wordRead.length() + matchIndex);
            }
        };
    }

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
