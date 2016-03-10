FROM ubuntu:15.10
RUN apt-get -y update && apt-get -y install build-essential cmake libqt4-dev git wget
RUN git clone https://github.com/BenjamenMeyer/simpleFileIndexer.git && cd simpleFileIndexer && mkdir build && cd build && cmake ../src && make
RUN mkdir test-data && cd test-data && wget http://www.gutenberg.org/ebooks/51353.txt.utf-8
RUN cd simpleFileIndexer/build && ./simpleFileIndexer /test-data/*
RUN echo "Test data in /test-dir" >> /etc/issue
WORKDIR /simpleFileIndexer/build
CMD ./simpleFileIndexer /test-data/*
