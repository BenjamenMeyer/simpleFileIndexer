#include <QtTest/QtTest>
#include <QStringList>

#include <fileIndexer.h>

class TestIndexer: public QObject
	{
	Q_OBJECT
	public:
		TestIndexer();
		~TestIndexer();
	private slots:
		void initTestCase();
		void cleanupTestCase();
		void init();
		void cleanup();

		// actual tests
		void test_buffer_parser_basic();
		void test_buffer_parser_basic_alt();
		void test_buffer_parser_basic_eof();

		void test_buffer_parser_numerics();
		void test_buffer_parser_numerics_alt();
		void test_buffer_parser_numerics_eof();

		void test_buffer_parser();
	};
TestIndexer::TestIndexer() : QObject(NULL)
	{
	}
TestIndexer::~TestIndexer()
	{
	}
void TestIndexer::initTestCase()
	{
	}
void TestIndexer::cleanupTestCase()
	{
	}
void TestIndexer::init()
	{
	}
void TestIndexer::cleanup()
	{
	}
void TestIndexer::test_buffer_parser_basic()
	{
	QString data;
	data += "hello";
	data += " ";
	data += "world";

	QString testFileName = "testing";

	WordCount results;
	processBuffer(testFileName, data, false, results);
	QVERIFY(results.contains("hello") == true);
	QVERIFY(results["hello"] == 1);
	QVERIFY(results.contains("world") == false);
	}
void TestIndexer::test_buffer_parser_basic_alt()
	{
	QString data;
	data += "hello";
	data += " ";
	data += "world";
	data += "\n";

	QString testFileName = "testing";

	WordCount results;
	processBuffer(testFileName, data, false, results);
	QVERIFY(results.contains("hello") == true);
	QVERIFY(results["hello"] == 1);
	QVERIFY(results.contains("world") == true);
	QVERIFY(results["world"] == 1);
	}
void TestIndexer::test_buffer_parser_basic_eof()
	{
	QString data;
	data += "hello";
	data += " ";
	data += "world";

	QString testFileName = "testing";

	WordCount results2;
	processBuffer(testFileName, data, true, results2);
	QVERIFY(results2.contains("hello") == true);
	QVERIFY(results2["hello"] == 1);
	QVERIFY(results2.contains("world") == true);
	QVERIFY(results2["world"] == 1);
	}
void TestIndexer::test_buffer_parser_numerics()
	{
	QString data;
	data += "90215";
	data += " ";
	data += "43768";

	QString testFileName = "testing";

	WordCount results;
	processBuffer(testFileName, data, false, results);
	QVERIFY(results.contains("90215") == true);
	QVERIFY(results["90215"] == 1);
	QVERIFY(results.contains("43768") == false);
	}
void TestIndexer::test_buffer_parser_numerics_alt()
	{
	QString data;
	data += "90215";
	data += " ";
	data += "43768";
	data += "\r";

	QString testFileName = "testing";

	WordCount results;
	processBuffer(testFileName, data, false, results);
	QVERIFY(results.contains("90215") == true);
	QVERIFY(results["90215"] == 1);
	QVERIFY(results.contains("43768") == true);
	QVERIFY(results["43768"] == 1);
	}
void TestIndexer::test_buffer_parser_numerics_eof()
	{
	QString data;
	data += "90215";
	data += " ";
	data += "43768";

	QString testFileName = "testing";

	WordCount results;
	processBuffer(testFileName, data, true, results);
	QVERIFY(results.contains("90215") == true);
	QVERIFY(results["90215"] == 1);
	QVERIFY(results.contains("43768") == true);
	QVERIFY(results["43768"] == 1);
	}
void TestIndexer::test_buffer_parser()
	{
	WordCount builder;
	builder.insert("hello", 10);
	builder.insert("world", 30);
	builder.insert("90215", 50);
	builder.insert("43768", 5);
	
	QStringList separators;
	separators << "\n" << "\r" << "è" << " " << "\t" << "\v";

	QString data;

	for (WordCount::const_iterator i = builder.constBegin(); i != builder.constEnd(); ++i)
		{
		uint64_t counter = 0;
		do
			{
			data.append(i.key());
			data.append(separators[counter % separators.length()]);
			++counter;
			} while (counter != i.value());
		}

	QString testFileName = "testing";
	WordCount results;
	processBuffer(testFileName, data, false, results);

	for (WordCount::const_iterator i = builder.constBegin(); i != builder.constEnd(); ++i)
		{
		QVERIFY(results.contains(i.key()) == true);
		QVERIFY(results[i.key()] == i.value());
		}
	QVERIFY(results.size() == builder.size());
	}

QTEST_MAIN(TestIndexer)
#include "test_index.moc"
