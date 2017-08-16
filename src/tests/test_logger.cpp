#include <QtCore/QObject>
#include <QtTest/QtTest>

#include <logger.h>

class TestLogger: public QObject
	{
	Q_OBJECT
	public:
		TestLogger();
		~TestLogger();
	
	private slots:
		void initTestCase();
		void cleanupTestCase();
		void init();
		void cleanup();

		// actual tests
		void default_logger_filename();
		void change_log_filename();
	};

TestLogger::TestLogger()
	{
	}
TestLogger::~TestLogger()
	{
	}
void TestLogger::initTestCase()
	{
	}
void TestLogger::cleanupTestCase()
	{
	}
void TestLogger::init()
	{
	}
void TestLogger::cleanup()
	{
	}
void TestLogger::default_logger_filename()
	{
	Logger logger;
	QVERIFY(logger.getLogFile() == QString(".application-logger.log"));
	}
void TestLogger::change_log_filename()
	{
	Logger logger;
	QString newLogFile = "where-is-waldo.log";
	logger.setLogFile(newLogFile);
	QVERIFY(logger.getLogFile() == newLogFile);
	}

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
