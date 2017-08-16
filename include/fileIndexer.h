#ifndef FILE_INDEXER_H__
#define FILE_INDEXER_H__

#include <stdint.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QFuture>

#include <logger.h>

//! Word Count Results
typedef QMap<QString, uint64_t> WordCount;
//! Delayed Word Count Result
typedef QFuture<WordCount> FutureWordCount;


/*! \brief File Processing Logging
 *
 *  Convenience method for logging messages for a given file being processed
 *
 *  \param _filename - the filename being processed
 *  \param _message - the log message to be recorded
 */
void resultDebugLog(QString _fileName, QString _message);

/*! \brief Increase the word count in the result
 *
 *  Convenience method for increading the count for a given word in the result
 *
 *  \param _results - result object to increase the count in
 *  \param wordToAdd - word the count is for
 *  \param _count - the count to increment by
 */
void addWord(WordCount& _results, QString wordToAdd, uint64_t _count=1);

/*! \brief Single File Word Indexing
 *
 *  Count the words in a given file
 *
 *  \param fileName - filename to process
 *
 *  \return WordCount object containing the counts of all words in the file
 */
WordCount indexFile(QString fileName);

/*! \brief Buffer Processing
 *
 *    Count the words contained within a QString data-buffer
 *
 *  \param fileName - the filename being processed
 *    \param buffer - data buffer to process
 *    \param allow_ending_word - if true, then consider a word that reaches the
 *        end of the buffer to be a word; if false, leave alone and return to
 *        the caller so more data can be added to the buffer
 *    \param results - WordCount object to update with the counts of the words found
 */
void processBuffer(const QString& fileName, QString& buffer, bool empty_buffer, WordCount& results);

/*! \brief Word Count MapReduce Accumulator
 *
 *  MapReduce splits out the processing between multiple workers. The accumulator combines the results
 *  back into a single result
 *
 *  \param _results - the final result object
 *  \param fileResult - the individual file results
 */
void indexFileReducer(WordCount& _results, const WordCount& fileResult);

/*! \brief File Processing Object
 *
 *  QObject to process the a file and generate the word counts
 */
class FileIndexer : QObject
    {
    Q_OBJECT
    public:
        /*! \brief Constructor
         *
         *  \param filesToAnalyze - the list of files to process
         *  \param _parent - parent QObject
         */
        FileIndexer(QStringList filesToAnalyze, QObject* _parent=NULL);
        /*! \brief Deconstructor
         */
        ~FileIndexer();

    public Q_SLOTS:
        /*! \brief Initialize the Indexer
         *
         *  Kickoff the indexing of all the files
         */
        void runIndexer();

        /*! \brief Log Capture Interface
         *
         *  Capture log messages from non-object sources
         *
         *  \param _type - Message Type (f.e Debug, Info, etc)
         *  \param _msg - Log Message
         */
        void incomingMessage(QtMsgType _type, QString _msg);

    Q_SIGNALS:
        /*! \brief Log Interface
         *
         *  Send a message to the log
         *
         *  \param _message Log Message to be recorded
         */
        void logMessage(QString _message);

        /*! \brief Ready for Application Termination
         *
         *  Signal that the processing is complete and all is
         *  ready for the application to be terminated.
         */
        void close();

    private:
        //! Thread for recording the log data
        QThread logThread;
        //! Log Recorder
        Logger theLog;

        //! Cumulative Indexing Results
        FutureWordCount anticipatedResults;

        //! List of files to be processed
        QStringList fileList;

    private Q_SLOTS:
        //! Notification the results are available
        void finalizeResults();
    };

#endif //FILE_INDEXER_H__
