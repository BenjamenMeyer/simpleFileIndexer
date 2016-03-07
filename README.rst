Simple File Indexer
===================
Simple Qt-based Distributed File Indexer

The software here will generate a 10-top word count on a list of
files provided to the application. It is a simple, command-line based
interface.

Strategy Taken
--------------

The dev task specification suggested to use a familiar technology; thus
as I am most familiar with C++/Qt I have chosen to use its functionality.
Qt makes for a straight forward, cross platform approach that is easy
to maintain, and update. The functionality here is built on Qt 4.8.

Qt 4.8 provides a nice functionally drive Mapped Reduce algorithm
implementation. For this implementation each file is processed by its
own worker thread, with the results being accumulated in the reduction
function. This design could easily be translated to other technologies
such as Apache Spark, OpenMP, Python TaskFlow, or other large scale
parallel processing APIs and Frameworks.

After the Map Reduce functionality calculates all the counts across
all the files, the list is converted to be numerically oriented, and
then the top 10 words are selected.

Alternative Strategies
----------------------

The strategy taken here is a simple strategy that works and works well
on a single computer. However, it can easily run into scaling issues
as it is limited to the resources of a single computer. Here are some
additional strategies that could be employed to utilize a network of
computers:

Option #1: Organize this into a master/slave architecture where multiple
slaves can access data and do the processing, reporting back the results
to the master.

Option #2: Implement using a technology such as OpenMP. This allows
easily setting up workers on local and remote machines, and handling the
communications between them.

Option #3: Write the implementation against a tool set stack like Apache Spark
or OpenStack's TaskFlow.

Each of these architectures have their advantages and tradeoffs. Option #1
means handling all the communications, devops, etc in the project. Option #2
off-loads some of the communications, but still requires a lot of work to
setup. Option #3 requires a lot of infrastructure (AMPQ, etc) to operate.

The strategy chosen using the Qt framework shows the architecture of the
solution while keeping the infrastructure simple, and code straight forward.

Building
--------
This project has the following dependencies in order to be built:

* A C++ Compiler with C++11 features
* Qt 4 Dev Environment, preferably Qt 4.8
* cmake 2.8 or later

On a Debian system, this can be achieved via the following:

.. code-block:: bash

	$ sudo apt-get install build-essential qt4-dev cmake

Building the software can be achieved via the following:

.. code-block:: bash

	$ mkdir build
	$ cd build
	$ cmake ../src
	$ make

Running
-------

Once built, the program is run from its build directory via the following:

.. code-block:: bash

	$ ./simpleFileIndexer <file #1> <file #2> ...

At present, each argument to the program is a file to be processed for
word counts.

**Note** One alternative would be to specify a directory and have it
process the entire directory. This can be somewhat done under Bash using
command-line completion techniques such as:

.. code-block:: bash

	$ ./simpleFileIndexer ../test-data/*

The only issue is the command-line limits.

Running with Docker
-------------------

There is also a docker image available:

.. code-block:: bash

	# docker run -it benjamenmeyer/simplefileindexer:latest bash

The image can be built using docker/Dockerfile in the repository.

Architecture as Implemented
---------------------------

The architecture of the solution is relatively straight forward:

* main initializes the software with the specified data set
* FileIndexer creates a series of threads using the functional interface
  to QtConcurrent::mappedReduce() to process the data, then refactor the
  data to find the result.
* QtConcurrent::mappedReduce() utilizes the QThreadPool to create a
  series of workers. Each worker processes a single file via indexFile(),
  and the results are compiled into a single result set via indexfFileReducer().
* For clarity, FileIndexer::runIndexer() starts the process, while
  FileIndexer::finalizeResults(). The split in functionality here also
  allows the results of QtConcurrent::mappedReduce() to be waited upon.
* All logging is done to a log file, and requires user output is generated
  to stdout/stderr as appropriate.
* C++'s std::locale functionality is used in order to enable the software to
  efficiently read the files one word at a time with minimal memory usage.
  If a character is not part of the official C++ locale specified to the
  iostream-based functionality (istream) then it is treated as whitespace, and
  thus word delimiting occurs. Valid words are A-Z, a-z, and 0-9; capitalization
  is ignored when calculating word counts.
* The final result is sent both to the log and to the console (stdout).

**Note** QtConcurrent::mappedReduce() reports that it could be waited upon;
however, I was not able to get it to achieve that result using the QFutureWatcher
interfaces. Thus the main thread will end up blocking when it goes to retrieve
the results. This is a place that could possible be improved to provide even
better performance in the future, and would be necessary to do if a more
complicated interface (such as a GUI) were provided.
