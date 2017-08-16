FIND_PACKAGE(Qt4 REQUIRED QtCore QtTest)

# relative locations of the headers and real source
SET (THE_INCLUDE_DIR ../../include)
SET (THE_SOURCE_DIR ..)

# grab all the headers files, and the source files
# except skip the `main.cpp` from the source listing
# since that won't be used for the unit tests as it is
# just the entry point for the rest of the program
FILE(GLOB header_files ${THE_INCLUDE_DIR}/*.h)
FILE(GLOB source_files ${THE_SOURCE_DIR}/*.cpp)
FILE(REMOVE source_files main.cpp)
SET (SOURCES ${source_files} ${header_files})

# find all the unit test files
FILE(GLOB test_files *.cpp)

# and build out the test suite based on the file names
MESSAGE(STATUS "Locating tests")
foreach(test_file IN LISTS test_files)

	MESSAGE(STATUS "Found test file ${test_file}")
	
	# Use the CPP file name as the test name
	# cmake doesn't give an easy way to do that, so use a couple
	# string replacements to drop the current source dir and the file ext
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" test_name ${test_file})
	string(REPLACE ".cpp" "" test_name ${test_name})
	MESSAGE(STATUS "	name: ${test_name} - file ${test_file}")

	ADD_EXECUTABLE(${test_name} ${test_file} ${SOURCES})
	TARGET_LINK_LIBRARIES(${test_name} ${QT_QTTEST_LIBRARY} ${QT_LIBRARIES})
	ADD_TEST(NAME ${test_name} COMMAND ${test_name})

endforeach(test_file)
MESSAGE(STATUS "Completed locating tests")
