#ifndef FILE_INDEXER_H__
#define FILE_INDEXER_H__

#include <stdint.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QFuture>

#include <logger.h>

// word count for a given result set (file or cumulative)
typedef QMap<QString, uint64_t> WordCount;
typedef QFuture<WordCount> FutureWordCount;

class FileIndexer : QObject
	{
	Q_OBJECT
	public:
		FileIndexer(QStringList filesToAnalyze, QObject* _parent=NULL);
		~FileIndexer();

	public Q_SLOTS:
		// indexer instantiator
		void runIndexer();

		// log capture from non-OO sources
		void incomingMessage(QtMsgType _type, QString _msg);

	Q_SIGNALS:
		// log to log file
		void logMessage(QString _message);

		// ready to close the application
		void close();

	private:
		// log file interface & thread
		QThread logThread;
		Logger theLog;

		// results store
		FutureWordCount anticipatedResults;

		// list of files specified by the user
		QStringList fileList;

	private Q_SLOTS:
		// results calculator and output
		void finalizeResults();
	};

#endif //FILE_INDEXER_H__
