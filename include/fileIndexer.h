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
