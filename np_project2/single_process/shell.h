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
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define FILE_MAX 1024
#define TOKEN_MAX 10000
#define BUFFER_SIZE 8192

using namespace std;

int my_shell(int);
void split_command(vector<string> &, string, char);
void clean_zombie();

#endif
