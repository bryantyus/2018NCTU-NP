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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

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
void broadcast(pid_t, int, char*, int, int);
void init_client(int, pid_t);
int readline(int, string &);
int connect_client(int);
void reset_client(pid_t);
client *get_client_by_id(int);
client *get_client_by_pid(pid_t);

#endif
