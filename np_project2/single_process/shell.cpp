#include "shell.h"
#include "socket.h"

extern client client_info[CLIENT_NUMBER];

static void env_handler(vector<string>, int);
static void process_handler(string, int *, int[][2], int, bool);
static void sigchld_handler(int);
static void handle_who(int);
static void handle_tell(int, int, char *);
static bool check_user_pipe(vector<string>);

int user_pipe[CLIENT_NUMBER + 1][CLIENT_NUMBER + 1][2];

static int checkname(int clientfd, char *name) {
  int ret = 1;

  for (int i = 1; i <= CLIENT_NUMBER; ++i) {
    if ((client_info[i].client_fd != clientfd) &&
        (!strcmp(client_info[i].name, name))) {
      ret = -1;
    }
  }
  return ret;
}

static void process_handler(string _command, int *cnt_line,
                            int cross_line_pipe_table[][2], int clientfd,
                            bool is_userpipe) {
  vector<string> command;
  char **token = (char **)calloc(TOKEN_MAX, sizeof(char *));
  char *filename;
  int next, in, out, err, fd, from_id = 0, to_id = 0;
  int arg = 0;
  int pipe_table[FILE_MAX][2] = {0};
  int process_count = 0;
  bool stdout_pipe = false;
  bool stderr_pipe = false;
  bool number_pipe = false;
  bool first_process = false;
  bool last_process = false;
  pid_t last_process_pid;
  client *client_ptr = get_client(clientfd);

  in = STDIN_FILENO;
  out = clientfd;
  err = clientfd;

  split_command(command, _command, ' ');
  command.push_back("end");
  memset(token, 0, sizeof(char *) * TOKEN_MAX);

  for (vector<string>::iterator it = command.begin(); it != command.end();) {
    while ((*it).c_str()[0] != '|' && (*it != "end") &&
           (*it).c_str()[0] != '!' && (*it != ">")) {
      // TODO: Can add condition to filter >N or <N and store them into another
      char *_token = strdup((*it).c_str());
      if ((_token[0] == '<' || _token[0] == '>') && _token[1] != '\0') {
        // Try to filter whole contain user_pipe information. e.g. >N or <N
        if (_token[0] == '<') {
          from_id = atoi(strdup((*it).substr(1, strlen(_token) - 1).c_str()));
        } else if (_token[0] == '>') {
          to_id = atoi(strdup((*it).substr(1, strlen(_token) - 1).c_str()));
        }
      } else {
        token[arg++] = _token;
      }
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
    cout << "LINE : " << __LINE__ << "\t\tprocess_count : " << process_count
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
    } else if (user_pipe[from_id][client_ptr->id][0]) {
      // There are some pipe from others.
      // TODO: user_pipe
      if (first_process) {
        in = user_pipe[from_id][client_ptr->id][0];
      }
      if (user_pipe[from_id][client_ptr->id][1]) {
        close(user_pipe[from_id][client_ptr->id][1]);
        user_pipe[from_id][client_ptr->id][1] = 0;
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
      // number pipe
      next = (*cnt_line +
              atoi(strdup(((*it).substr(1, (*it).size() - 1)).c_str()))) %
             FILE_MAX;

      if ((*it).c_str()[0] == '!') {
        stderr_pipe = true;
      }
      number_pipe = true;
      err = clientfd;
      out = clientfd;
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

      if (to_id) {
        pipe(user_pipe[client_ptr->id][to_id]);
        out = user_pipe[client_ptr->id][to_id][1];
        err = user_pipe[client_ptr->id][to_id][1];
      } else {
        out = clientfd;
        err = clientfd;
      }
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
      if (err != STDERR_FILENO) {
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
      int status;

      if (last_process) {
        last_process_pid = pid;
      }

      if (!number_pipe && last_process && to_id == 0) {
        waitpid(last_process_pid, &status, 0);
      } else if (is_userpipe && !last_process) {
        waitpid(pid, &status, 0);
      }
      /*
else if (is_userpipe && to_id && last_process)
      {
waitpid(last_process_pid, &status, 0);
      }
*/

      memset(token, 0, sizeof(char *) * TOKEN_MAX);

      arg = 0;
      next = 0;
      in = STDIN_FILENO;
      out = clientfd;
      err = clientfd;

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
      if (from_id) {
        if (user_pipe[from_id][client_ptr->id][0]) {
          close(user_pipe[from_id][client_ptr->id][0]);
          user_pipe[from_id][client_ptr->id][0] = 0;
        }
        if (user_pipe[from_id][client_ptr->id][1]) {
          close(user_pipe[from_id][client_ptr->id][1]);
          user_pipe[from_id][client_ptr->id][1] = 0;
        }
      }

      if (to_id) {
        if (user_pipe[client_ptr->id][to_id][1]) {
          close(user_pipe[client_ptr->id][to_id][1]);
          user_pipe[client_ptr->id][to_id][1] = 0;
        }
      }
    }
  }
  free(token);
}

int my_shell(int clientfd) {

  char del = ' ';
  int n;
  client *client_ptr = get_client(clientfd);

  setenv("PATH", client_ptr->path, 1);

  string buff;
  vector<string> command;

  if ((n = readline(clientfd, buff)) <= 0) {
    if (n == 0) {
      // didn't receieve the message.
      return 1;
    } else {
      return -1;
    }
  }

  split_command(command, buff, del);
  client_ptr->cnt_line = (client_ptr->cnt_line + 1) % FILE_MAX;

  if (command[0] == "setenv" || command[0] == "printenv") {
    env_handler(command, clientfd);
  } else if (command[0] == "exit") {
    // TODO: clean up all user_pipe about clientfd
    return -1;
  } else if (command[0] == "who") {
    handle_who(clientfd);
  } else if (command[0] == "tell") {
    int to_id = atoi(strdup(command[1].c_str()));
    char *tell_message;
    strtok_r(strdup(buff.c_str()), " ", &tell_message);
    strtok_r(NULL, " ", &tell_message);
    handle_tell(clientfd, to_id, tell_message);
  } else if (command[0] == "yell") {
    char *yell_message;
    strtok_r(strdup(buff.c_str()), " ", &yell_message);
    broadcast(clientfd, YELL, yell_message, -1, -1);
  } else if (command[0] == "name") {
    char *new_name;
    strtok_r(strdup(buff.c_str()), " ", &new_name);
    if (checkname(clientfd, new_name) > 0) {
      memset(client_ptr->name, 0, sizeof(client_ptr->name));
      strncpy(client_ptr->name, new_name, strlen(new_name));
      broadcast(clientfd, CHANGENAME, new_name, -1, -1);
    } else {
      char temp[BUFFER_SIZE] = {0};
      sprintf(temp, "*** User '%s' already exists. ***\n", new_name);
      send(clientfd, temp, strlen(temp), 0);
    }
  } else if (check_user_pipe(command)) {
    // handle user_pipe
    // TODO: handle err message.
    int from_id = 0, to_id = 0;
    bool receive_pipe = false;
    bool sendto_pipe = false;
    char temp[BUFFER_SIZE] = {0};

    for (vector<string>::iterator it = command.begin(); it != command.end();
         ++it) {
      char *p = strdup((*it).c_str());
      if ((p[0] == '>' || p[0] == '<') && (strlen(p) >= 2)) {
        if (p[0] == '>') {
          sendto_pipe = true;
          to_id = atoi(strdup((*it).substr(1, strlen(p) - 1).c_str()));
        } else if (p[0] == '<') {
          receive_pipe = true;
          from_id = atoi(strdup((*it).substr(1, strlen(p) - 1).c_str()));
        }
      }
    }

    if (receive_pipe) {
      if (get_client_by_id(from_id) == NULL) {
        // The client doesn't exist.
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "*** Error: user #%d does not exist yet. ***\n", from_id);
        send(clientfd, temp, strlen(temp), 0);
        write(clientfd, "% ", 2);
        return 1;
      } else if (user_pipe[from_id][client_ptr->id][0] == 0) {
        // pipe doesn't exist.
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "*** Error: the pipe #%d->#%d does not exist yet. ***\n",
                from_id, client_ptr->id);
        send(clientfd, temp, strlen(temp), 0);
        write(clientfd, "% ", 2);
        return 1;
      } else {
        char *_temp = (char *)malloc(sizeof(char) * BUFFER_SIZE);
        sprintf(_temp, "%s", strdup(buff.c_str()));
        broadcast(clientfd, RECEIVEPIPE, _temp, from_id, client_ptr->id);
        free(_temp);
      }
    }
    if (sendto_pipe) {
      if (get_client_by_id(to_id) == NULL) {
        // The client does not exist.
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "*** Error: user #%d does not exist yet. ***\n", to_id);
        send(clientfd, temp, strlen(temp), 0);
        write(clientfd, "% ", 2);
        return 1;
      } else if (user_pipe[client_ptr->id][to_id][0] ||
                 user_pipe[client_ptr->id][to_id][1]) {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "*** Error: the pipe #%d->#%d already exists. ***\n",
                client_ptr->id, to_id);
        send(clientfd, temp, strlen(temp), 0);
        write(clientfd, "% ", 2);
        return 1;
      } else {
        client *send_client = get_client_by_id(to_id);
        broadcast(clientfd, SENDPIPE, strdup(buff.c_str()), -1, to_id);
      }
    }
    // TODO: check whether we want to output client exists.

    process_handler(buff, &(client_ptr->cnt_line),
                    client_ptr->cross_line_pipe_table, clientfd, true);
  } else {
    process_handler(buff, &(client_ptr->cnt_line),
                    client_ptr->cross_line_pipe_table, clientfd, false);
  }

  write(clientfd, "% ", 2);

  return 1;
}

void split_command(vector<string> &command, string input, char del) {
  stringstream ss(input);
  string item;
  // Make input as a standard input stream.
  while (getline(ss, item, del)) {
    command.push_back(item);
  }
}

static void env_handler(vector<string> command, int clientfd) {
  char *env_var;
  client *client_ptr = get_client(clientfd);

  if (command[0] == "setenv") {
    memset(client_ptr->path, 0, sizeof(client_ptr->path));
    strncpy(client_ptr->path, strdup(command[2].c_str()),
            strlen(strdup(command[2].c_str())));
  } else {
    env_var = getenv(command[1].c_str());
    if (!env_var) {
      return;
    } else {
      strncat(env_var, "\n", 1);
      write(clientfd, env_var, strlen(env_var));
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

static void handle_who(int owner_clientfd) {
  char message[5000] = {0};
  int fd;
  client *client_ptr;

  sprintf(message, "<ID>\t<nickname>\t<IP/port>\t<indicate me>\n");
  for (int i = 1; i <= CLIENT_NUMBER; ++i) {
    if ((fd = client_info[i].client_fd)) {
      if (fd == owner_clientfd) {
        char temp[500] = {0};
        sprintf(temp, "%d\t%s\tCGILAB/511\t<-me\n", client_info[i].id,
                client_info[i].name);
        strncat(message, temp, strlen(temp));
      } else {
        char temp[500] = {0};
        sprintf(temp, "%d\t%s\tCGILAB/511\t\n", client_info[i].id,
                client_info[i].name);
        strncat(message, temp, strlen(temp));
      }
    }
  }
  send(owner_clientfd, message, strlen(message), 0);
}

static void handle_tell(int from_fd, int to_id, char *message) {
  bool found = false;
  char _message[500] = {0};
  int receiver;
  client *sender = get_client(from_fd);
  client *rec = get_client_by_id(to_id);

  if (rec != NULL && rec->client_fd > 0) {
    found = true;
    receiver = rec->client_fd;
  }

  if (found) {
    sprintf(_message, "*** %s told you ***: %s\n", sender->name, message);
    send(receiver, _message, strlen(_message), 0);
  } else {
    sprintf(_message, "*** Error: user #%d does not exist yet. ***\n", to_id);
    send(from_fd, _message, strlen(_message), 0);
  }
}

static bool check_user_pipe(vector<string> command) {
  for (vector<string>::iterator it = command.begin(); it != command.end();
       ++it) {
    char *p = strdup((*it).c_str());
    if ((p[0] == '>' || p[0] == '<') && p[1] != '\0') {
      return true;
    }
  }
  return false;
}
