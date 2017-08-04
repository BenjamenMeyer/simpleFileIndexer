#ifndef MY_LOGGER_H__
#define MY_LOGGER_H__

#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>

/*! \brief Log Recording
 *
 *  Basic log system to record all log messages
 *  to a file asynchronously.
 */
class Logger: public QObject
    {
    Q_OBJECT
    public:
        /*! \brief Constructor
         *
         *  \param _parent - parent QObject
         */
        Logger(QObject* _parent=NULL);
        /*! \brief Deconstructor
         */
        virtual ~Logger();

        /*! \brief set the log filename
         *
         *  Set the file name to record the log data to
         */
        void setLogFile(QString _filename);
        /*! \brief Log filename
         *
         *  \return log filename
         */
        QString getLogFile() const;

    public Q_SLOTS:
        /*! \brief Log Message
         *
         *  Send a message to the log
         *
         *  \param _message Log Message to be recorded
         */
        void message(QString _message);

        /*! \brief Ready for Application Termination
         *
         *  Application is ready to be closed and therefore
         *  the log file needs to be closed.
         */
        void closeApplication();

    Q_SIGNALS:
        /*! \brief Log Message
         *
         *  Send a message to the log
         *
         *  \param _message Log Message to be recorded
         */
        void send_message(QString _message);

    protected:
        //! Log file used to record the data
        QFile logFile;
    };

#endif //MY_LOGGER_H__

