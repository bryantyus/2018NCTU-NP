#ifndef __SHELL_H__
#define __SHELL_H__

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
#include <sys/mman.h>
#include <sys/stat.h>


#define FILE_MAX 1024
#define TOKEN_MAX 10000
#define BUFFER_SIZE 16384


using namespace std;

void my_shell(int, pid_t);
void split_command(vector<string> &, string, char);
void clean_zombie();
void *get_shared_memory(int, size_t);
void recv_broadcast();
void sigusr1_handler(int);

#endif

