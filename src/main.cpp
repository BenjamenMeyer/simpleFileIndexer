#include <QCoreApplication>

#include <fileIndexer.h>

#include <iostream>

int main(int argc, char* argv[])
	{
	QCoreApplication theApplication(argc, argv);

	// check the number of args, if only 1 is specified
	// then generate the usage as the user failed to specify
	// at least 1 file to process
	if (argc < 2)
		{
		std::cerr << "Invalid parameter" << std::endl;
		std::cerr << argv[0] << " [<file list>] " << std::endl;
		return 1;
		}

	// all args after the first are files to be processed
	QStringList filesToProcess;
	for (int i=1; i < argc; ++i)
		{
		filesToProcess << QString(argv[i]);
		std::cout << "Found file: " << argv[i] << std::endl;
		}

	// create an index of the indexer; it'll start running
	// as soon as the event loop kicks off
	FileIndexer main(filesToProcess, NULL);

	// and start the event loop
	return theApplication.exec();
	}
