#ifndef __SOCKET_H__
#define __SOCKET_H__


#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <iterator>
#include <queue>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

void socket_connect(int *);
int readline(int, string &);
int connect_client(int);

#endif
