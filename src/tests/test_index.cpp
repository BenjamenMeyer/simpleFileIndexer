#include <QtTest/QtTest>
#include <QStringList>
#include <QtGlobal>

#include <fileIndexer.h>

int get_random_value(int maxValue=-1)
    {
    const int MAX_VALUE = 1000;

    if (maxValue == -1)
        {
        maxValue = MAX_VALUE;
        }

    int r = qrand();
    return (r % maxValue);
    }

void generate_word_counts(WordCount& worker, const QStringList& words, int iter_cap)
    {
    int maxWorker = get_random_value(iter_cap);
    for (int i = 0; i < maxWorker; ++i)
        {
        QString word = words[i % words.size()];
        worker[word] += get_random_value(10);
        }
    }

void combine_counts(WordCount& destination, const WordCount& source)
    {
    for (WordCount::const_iterator i = source.constBegin(); i != source.constEnd(); ++i)
        {
        if (destination.contains(i.key()) == true)
            {
            destination[i.key()] += i.value();
            }
        else
            {
            destination.insert(i.key(), i.value());
            }
        }
    }

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

        void test_counter();
        void test_reducer();
        void test_capitalization();
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
void TestIndexer::test_counter()
    {
    WordCount checker;

    QString wordToCount = "kanji";
    QVERIFY(checker.contains(wordToCount) == false);
    QVERIFY(checker.size() == 0);

    addWord(checker, wordToCount);
    QVERIFY(checker.size() == 1);
    QVERIFY(checker.contains(wordToCount) == true);
    QVERIFY(checker[wordToCount] == 1);

    addWord(checker, wordToCount, 9);    // total should be 10
    QVERIFY(checker.size() == 1);
    QVERIFY(checker[wordToCount] == 10);

    QString anotherWordToCount = "katakana";
    QVERIFY(checker.contains(anotherWordToCount) == false);
    addWord(checker, anotherWordToCount, 20);
    QVERIFY(checker.size() == 2);
    QVERIFY(checker.contains(anotherWordToCount) == true);
    QVERIFY(checker[anotherWordToCount] == 20);
    }

void TestIndexer::test_reducer()
    {
    // testing word list
    QStringList words;
    words << "phoentic" << "syllabic" << "logographic" << "grapheme" << "phoneme";

    // simulate the results from 3 workers
    WordCount worker1;
    WordCount worker2;
    WordCount worker3;
    generate_word_counts(worker1, words, 50);
    generate_word_counts(worker2, words, 150);
    generate_word_counts(worker3, words, get_random_value(1000));

    WordCount finalCount;
    QVERIFY(finalCount.size() == 0);
    indexFileReducer(finalCount, worker1);
    for (WordCount::const_iterator i = worker1.constBegin(); i != worker1.constEnd(); ++i)
        {
        QVERIFY(finalCount.contains(i.key()) == true);
        QVERIFY(finalCount[i.key()] == i.value());
        }

    indexFileReducer(finalCount, worker2);
    WordCount worker1and2;
    combine_counts(worker1and2, worker1);
    combine_counts(worker1and2, worker2);
    for (WordCount::const_iterator i = worker1and2.constBegin(); i != worker1and2.constEnd(); ++i)
        {
        QVERIFY(finalCount.contains(i.key()) == true);
        QVERIFY(finalCount[i.key()] == i.value());
        }

    indexFileReducer(finalCount, worker3);
    WordCount workerAll;
    combine_counts(workerAll, worker1);
    combine_counts(workerAll, worker2);
    combine_counts(workerAll, worker3);
    for (WordCount::const_iterator i = workerAll.constBegin(); i != workerAll.constEnd(); ++i)
        {
        QVERIFY(finalCount.contains(i.key()) == true);
        QVERIFY(finalCount[i.key()] == i.value());
        }
    }
void TestIndexer::test_capitalization()
    {
    // testing word list
    QStringList words;
    words << "alpha" << "Alpha" << "aLpha" << "ALpha" << "alPHA";

    WordCount checker;
    uint64_t total = 0;
    for (QStringList::const_iterator i = words.constBegin(); i != words.constEnd(); ++i)
        {
        int new_counts = get_random_value();
        total += new_counts;
        addWord(checker, *i, new_counts);
        }
    QVERIFY(checker.size() == 1);
    QVERIFY(checker.contains("alpha") == true);
    QVERIFY(checker["alpha"] == total);
    }

QTEST_MAIN(TestIndexer)
#include "test_index.moc"
