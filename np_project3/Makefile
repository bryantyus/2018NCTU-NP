# This Makefile is used to compile console.cpp and httpserver.cpp to console.cgi and http_server separately.

OBJ		= http_server.o console.o
CC		= g++
CFLAGS	= -std=c++11 -g -Wall -O2 -lboost_system -pthread -pedantic
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))


all: $(OBJ)
	$(CC) -o http_server http_server.o $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CFLAGS)
	$(CC) -o console.cgi console.o $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	@rm *.o http_server console.cgi
