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


#define FILE_MAX 1024
#define TOKEN_MAX 10000
#define BUFFER_SIZE 8192

using namespace std;

void my_shell(string, int *, int [][2]);
void split_command(vector<string> &, string, char);
void clean_zombie();
int readline(string &);

#endif

