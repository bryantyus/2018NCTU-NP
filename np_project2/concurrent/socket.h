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
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define CLIENT_NUMBER 30

#define LOGIN 1
#define LOGOUT 2
#define YELL 3
#define CHANGENAME 4
#define RECEIVEPIPE 5
#define SENDPIPE 6

typedef struct _client {
  char name[50];
  char ip_address[50];
  uint16_t port;
  int id;
  int client_fd;
  int cnt_line;
  int cross_line_pipe_table[1024][2];
  int enable;
  pid_t pid;
} client;

using namespace std;

void socket_connect(int *);
void broadcast(pid_t, int, char *, int, int);
void init_client(int, pid_t);
int readline(int, string &);
int connect_client(int);
void reset_client(pid_t);
client *get_client_by_id(int);
client *get_client_by_pid(pid_t);

#endif
