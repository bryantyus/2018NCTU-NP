#ifndef __SHELL_H__
#define __SHELL_H__

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <queue>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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
