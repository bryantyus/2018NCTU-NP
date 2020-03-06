#include "shell.h"

static void env_handler(vector<string>);
static void process_handler(string, int *, int[][2]);
static void sigchld_handler(int);

static void process_handler(string _command, int *cnt_line,
                            int cross_line_pipe_table[][2]) {
  vector<string> command;
  char **token = (char **)calloc(TOKEN_MAX, sizeof(char *));
  char *filename;
  int next, in, out, err, fd;
  int arg = 0;
  int pipe_table[FILE_MAX][2] = {0};
  int process_count = 0;
  bool stdout_pipe = false;
  bool stderr_pipe = false;
  bool number_pipe = false;
  bool first_process = false;
  bool last_process = false;
  pid_t last_process_pid;

  in = STDIN_FILENO;
  out = STDOUT_FILENO;
  err = STDERR_FILENO;

  split_command(command, _command, ' ');
  command.push_back("end");
  memset(token, 0, sizeof(char *) * TOKEN_MAX);

  for (vector<string>::iterator it = command.begin(); it != command.end();) {
    while ((*it).c_str()[0] != '|' && (*it != "end") &&
           (*it).c_str()[0] != '!' && (*it != ">")) {
      token[arg++] = strdup((*it).c_str());
      it++;
      continue;
    }
    token[arg] = NULL;

    if (*it == "end" && number_pipe) {
      break;
    } else {
      process_count = (process_count + 1) % FILE_MAX;
    }

    if (process_count == 1) {
      first_process = true;
    } else {
      first_process = false;
    }

#ifdef DEBUG
    cout << "---------------------------------------------------" << endl;
    cout << "LINE : " << __LINE__ << "\t\tprocess_count : " << *process_count
         << endl;
    cout << "---------------------------------------------------" << endl;
#endif
    if (cross_line_pipe_table[*cnt_line][0]) {
      if (first_process) {
        in = cross_line_pipe_table[*cnt_line][0];
      }
      if (cross_line_pipe_table[*cnt_line][1]) {
        close(cross_line_pipe_table[*cnt_line][1]);
        cross_line_pipe_table[*cnt_line][1] = 0;
      }
    } else {
      in = pipe_table[process_count][0];
      if (pipe_table[process_count][1]) {
        close(pipe_table[process_count][1]);
        pipe_table[process_count][1] = 0;
      }
    }

    if (((*it).c_str()[0] == '|' || (*it).c_str()[0] == '!') &&
        (*it).c_str()[1]) {
      next = (*cnt_line +
              atoi(strdup(((*it).substr(1, (*it).size() - 1)).c_str()))) %
             FILE_MAX;

      if ((*it).c_str()[0] == '!') {
        stderr_pipe = true;
      }
      number_pipe = true;
      it++;

#ifdef DEBUG
      cout << "---------------------------------------------------" << endl;
      cout << "executing number_piped in LINE : " << __LINE__ << endl;
      cout << "---------------------------------------------------" << endl;
#endif
    } else if (((*it).c_str()[0] == '|' || (*it).c_str()[0] == '!') &&
               (*it).c_str()[1] == '\0') {
      // Ordinary pipe.
      it++;
      next = (process_count + 1) % FILE_MAX;
      stdout_pipe = true;
#ifdef DEBUG
      cout << "---------------------------------------------------" << endl;
      cout << "executing ordinary_piped in LINE : " << __LINE__ << endl;
      cout << "---------------------------------------------------" << endl;
#endif
    } else if ((*it).c_str()[0] == '>') {
      it++;
      filename = strdup((*it).c_str());
      it++;
      if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
        perror("open");
      }
      out = fd;
      last_process = true;
    } else {
      // "end"
      if (*it == "end") {
        last_process = true;
      }
      it++;
    }

    if (pipe_table[next][1] == 0 && stdout_pipe) {
      if (pipe(pipe_table[next]) < 0) {
        perror("pipe");
      }
      stdout_pipe = false;
      out = pipe_table[next][1];
    }

    if (number_pipe) {
      if (cross_line_pipe_table[next][1] == 0) {
        if (pipe(cross_line_pipe_table[next]) < 0) {
          perror("pipe");
        }
      }
      out = cross_line_pipe_table[next][1];
      if (stderr_pipe) {
        err = cross_line_pipe_table[next][1];
      }
    }

    clean_zombie();
    pid_t pid;
    while ((pid = fork()) < 0) {
      usleep(1000);
    }
    if (pid == 0) {
      if (in != STDIN_FILENO) {
        dup2(in, STDIN_FILENO);
        close(in);
      }
      if (stderr_pipe) {
        dup2(err, STDERR_FILENO);
      }
      if (out != STDOUT_FILENO) {
        dup2(out, STDOUT_FILENO);
        close(out);
      }
      execvp(token[0], token);
      fprintf(stderr, "Unknown command: [%s].\n", token[0]);
      exit(1);
    } else {
      if (last_process) {
        last_process_pid = pid;
      }

      if (!number_pipe && last_process) {
        int status;
        waitpid(last_process_pid, &status, 0);
      }

      //            wait(&status);
      memset(token, 0, sizeof(char *) * TOKEN_MAX);

      arg = 0;
      next = 0;
      in = STDIN_FILENO;
      out = STDOUT_FILENO;
      err = STDERR_FILENO;

      if (pipe_table[process_count][0]) {
        close(pipe_table[process_count][0]);
        pipe_table[process_count][0] = 0;
      }
      if (pipe_table[process_count][1]) {
        close(pipe_table[process_count][1]);
        pipe_table[process_count][1] = 0;
      }
      if (cross_line_pipe_table[*cnt_line][0]) {
        close(cross_line_pipe_table[*cnt_line][0]);
        cross_line_pipe_table[*cnt_line][0] = 0;
      }
      if (cross_line_pipe_table[*cnt_line][1]) {
        close(cross_line_pipe_table[*cnt_line][1]);
        cross_line_pipe_table[*cnt_line][1] = 0;
      }
    }
  }
  free(token);
}

void my_shell(string input, int *cnt_line, int cross_line_pipe_table[][2]) {

  char del = ' ';

  vector<string> command;
  split_command(command, input, del);

  *cnt_line = (*cnt_line + 1) % FILE_MAX;

  if (command[0] == "setenv" || command[0] == "printenv") {
    env_handler(command);
  } else if (command[0] == "exit") {
    exit(0);
  } else {
    process_handler(input, cnt_line, cross_line_pipe_table);
  }
}

void split_command(vector<string> &command, string input, char del) {
  stringstream ss(input);
  string item;
  // Make input as a standard input stream.
  while (getline(ss, item, del)) {
    command.push_back(item);
  }
}

static void env_handler(vector<string> command) {
  char *env_var;
  if (command[0] == "setenv") {
    if (setenv(command[1].c_str(), command[2].c_str(), 1) < 0) {
      perror("setenv");
    }
  } else {
    env_var = getenv(command[1].c_str());
    if (!env_var) {
      return;
    } else {
      cout << env_var << endl;
    }
  }
}

static void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

void clean_zombie() {
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}

int readline(string &command) {
  int buffersize = BUFFER_SIZE;
  int c, position = 0;
  char *buffer = (char *)malloc(sizeof(char) * buffersize);

  if (!buffer) {
    fprintf(stderr, "npshell: allocation err\n");
    exit(1);
  }

  while (1) {
    c = getchar();
    if (c == '\n' || c == '\r') {
      buffer[position] = '\0';
      command.assign(buffer, position);
      free(buffer);
      return position;
    } else if (c == EOF) {
      free(buffer);
      return -1;
    } else {
      buffer[position++] = c;
    }
    if (position >= buffersize) {
      buffersize += BUFFER_SIZE;
      buffer = (char *)realloc(buffer, buffersize);
      if (!buffer) {
        fprintf(stderr, "allocation err\n");
        exit(1);
      }
    }
  }
}
