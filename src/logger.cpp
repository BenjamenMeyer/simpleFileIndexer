#include <logger.h>

#include <QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QTimer>

Logger::Logger(QObject* _parent): QObject(_parent)
	{
	// default log file
	setLogFile(QString(".application-logger.log"));
	}
Logger::~Logger()
	{
	// close the log file before the object is destroyed
	if (logFile.isOpen())
		{
		logFile.close();
		}
	}

void Logger::setLogFile(QString _filename)
	{
	// switch log files

	// create a list with the new log file name and the existing log file name
	// then try to open the files and use the first one that opens. This is done
	// in order to try to guarantee at least one file will be opened.
	QStringList logFileOptions;
	logFileOptions << _filename << logFile.fileName();

	// close any existing log file
	if (logFile.isOpen())
		{
		logFile.close();
		}

	// find the new log file to use, hopefully the first entry in the list
	for (QStringList::iterator iter = logFileOptions.begin(); iter != logFileOptions.end(); ++iter)
		{
		if (!(*iter).isEmpty())
			{
			logFile.setFileName((*iter));
			// Text is used because some platforms (Windows) require it for best file interaction
			// WriteOnly is used since the file will not be read by the application
			// Append is used to allow the file to retain existing data
			// Unbuffered is used to try to make sure all the data is written quickly just like stderr so nothing is lost
			if (logFile.open(QIODevice::Text|QIODevice::Append|QIODevice::WriteOnly|QIODevice::Unbuffered))
				{
				break;
				}
			}
		}
	
	// record success or failure
	if (logFile.isOpen())
		{
		Q_EMIT send_message(QString("Recording log to file %1").arg(logFile.fileName()));
		}
	else
		{
		Q_EMIT send_message("No log file to record in use");
		}
	}

QString Logger::getLogFile() const
	{
    // return the filename of the active log file
	return logFile.fileName();
	}

void Logger::message(QString _message)
	{
	// if the log file is opened, then record the message
	if (logFile.isOpen())
		{
		logFile.write(_message.toLatin1());
		logFile.write("\n");
		}
	}

void Logger::closeApplication()
	{
	// close the application after 1 second
	QTimer::singleShot(1000, QCoreApplication::instance(), SLOT(quit())); 
	}
