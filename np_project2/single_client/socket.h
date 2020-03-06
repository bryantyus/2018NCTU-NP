#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <queue>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

void socket_connect(int *);
int readline(int, string &);
int connect_client(int);

#endif
