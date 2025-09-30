CXX=g++
CXXFLAGS=-std=c++17 -g -pedantic -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer
LDLIBS=

all: clean server client

server: server.cpp common.cpp FIFORequestChannel.cpp
	$(CXX) $(CXXFLAGS) -o server $^ $(LDLIBS)

client: client.cpp common.cpp FIFORequestChannel.cpp
	$(CXX) $(CXXFLAGS) -o client $^ $(LDLIBS)

.PHONY: clean test

clean:
	rm -f server client fifo* data*_* *.tst *.o *.csv received/*

test: all
	chmod u+x pa1-tests.sh
	./pa1-tests.sh