#include "shell.h"
#include "socket.h"


extern int (*user_pipe)[CLIENT_NUMBER + 1][2];
extern char *share_message;
extern client *client_info;


static void env_handler(vector<string>, int);
static void process_handler(string, int *, int [][2], pid_t, bool);
static void sigchld_handler(int);
static void handle_who(client *);
static void handle_tell(client *, int, char*);
static bool check_user_pipe(vector<string>);


static int checkname(pid_t my_pid, char *name)
{
    int ret = 1;

    for (int i = 1; i <= CLIENT_NUMBER; ++i)
    {
        if ((client_info[i].pid != my_pid) && (!strcmp(client_info[i].name, name)))
        {
            ret = -1;
        }
    }
    return ret;
}

static void process_handler(string _command, int *cnt_line, int cross_line_pipe_table[][2], pid_t client_pid, bool is_userpipe)
{
    vector<string> command;
    char **token = (char **)calloc(TOKEN_MAX, sizeof(char*));
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
	client *client_ptr = get_client_by_pid(client_pid);
	int clientfd = client_ptr->client_fd;


    in  = STDIN_FILENO;
    out = clientfd;
    err = clientfd;

    split_command(command, _command, ' ');
    command.push_back("end");
    memset(token, 0, sizeof(char*) * TOKEN_MAX);

    for (vector<string>::iterator it = command.begin(); it != command.end();)
    {
        while ((*it).c_str()[0] != '|' && (*it != "end")  && (*it).c_str()[0] != '!' && (*it != ">"))
        {
			char *_token = strdup((*it).c_str());
			if ((_token[0] == '<' || _token[0] == '>') && _token[1] != '\0')
			{
				// Try to filter whole contain user_pipe information. e.g. >N or <N
				if (_token[0] == '<')
				{
					from_id = atoi(strdup((*it).substr(1, strlen(_token) - 1).c_str()));
				}
				else if (_token[0] == '>')
				{
					to_id = atoi(strdup((*it).substr(1, strlen(_token) - 1).c_str()));
				}
			}
			else
			{
				token[arg++] = _token;
			}
            it++;
            continue;
        }
        token[arg] = NULL;

        if (*it == "end" && number_pipe)
        {
            break;
        }
        else
        {
            process_count = (process_count + 1) % FILE_MAX;
        }

        if (process_count == 1)
        {
            first_process = true;
        }
        else
        {
            first_process = false;
        }

#ifdef DEBUG
        cout << "---------------------------------------------------" << endl;
        cout << "LINE : " << __LINE__ << "\t\tprocess_count : " << process_count << endl;
        cout << "---------------------------------------------------" << endl;
#endif
		if (user_pipe[from_id][client_ptr->id][0] != 0)
		{
			// There are some pipe from others.
			if (first_process)
			{
				in = user_pipe[from_id][client_ptr->id][0];
			}
			if (user_pipe[from_id][client_ptr->id][1] != 0)
			{
				close(user_pipe[from_id][client_ptr->id][1]);
				user_pipe[from_id][client_ptr->id][1] = 0;
			}
		}
		else if (cross_line_pipe_table[*cnt_line][0])
        {
            if (first_process)
            {
                in = cross_line_pipe_table[*cnt_line][0];
            }
            if (cross_line_pipe_table[*cnt_line][1])
            {
                close(cross_line_pipe_table[*cnt_line][1]);
                cross_line_pipe_table[*cnt_line][1] = 0;
            }
        }
        else
        {
            in = pipe_table[process_count][0];
            if (pipe_table[process_count][1])
            {
                close(pipe_table[process_count][1]);
                pipe_table[process_count][1] = 0;
            }
        }

        if ( ((*it).c_str()[0] == '|' || (*it).c_str()[0] == '!') && (*it).c_str()[1] )
        {
			// number pipe
            next = (*cnt_line + atoi(strdup(((*it).substr(1, (*it).size() - 1)).c_str()))) % FILE_MAX;

			if ((*it).c_str()[0] == '!')
			{
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
        }
        else if (((*it).c_str()[0] == '|' || (*it).c_str()[0] == '!') && (*it).c_str()[1] == '\0')
        {
            // Ordinary pipe.
            it++;
            next = (process_count + 1) % FILE_MAX;
            stdout_pipe = true;
#ifdef DEBUG        
            cout << "---------------------------------------------------" << endl;
            cout << "executing ordinary_piped in LINE : " << __LINE__ << endl;
            cout << "---------------------------------------------------" << endl;
#endif
        }
        else if ((*it).c_str()[0] == '>')
        {
            it++;
            filename = strdup((*it).c_str());
            it++;
            if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
            {
                perror("open");
            }
            out = fd;
            last_process = true;
        }
        else
        {
            // "end"
            if (*it == "end")
            {
                last_process = true;
            }
            it++;

			if (to_id && (user_pipe[client_ptr->id][to_id][0] == 0 && user_pipe[client_ptr->id][to_id][1] == 0))
			{
				pipe(user_pipe[client_ptr->id][to_id]);
				out = user_pipe[client_ptr->id][to_id][1];
				err = user_pipe[client_ptr->id][to_id][1];
			}
			else
			{
				out = clientfd;
				err = clientfd;
			}
        }

        if (pipe_table[next][1] == 0 && stdout_pipe)
        {
            pipe(pipe_table[next]);
            stdout_pipe = false;
            out = pipe_table[next][1];
        }

		if (number_pipe)
		{
			if (cross_line_pipe_table[next][1] == 0)
			{
				pipe(cross_line_pipe_table[next]);
			}
			out = cross_line_pipe_table[next][1];
			if (stderr_pipe)
			{
				err = cross_line_pipe_table[next][1];
			}
		}

        clean_zombie();
        pid_t pid;
        while ((pid = fork()) < 0)
        {
            usleep(1000);
        }
        if (pid == 0)
        {
            if (in != STDIN_FILENO)
            {
                dup2(in, STDIN_FILENO);
                if (close(in) < 0)
				{
					perror("close in");
				}
            }
			if (err != STDERR_FILENO)
			{
				dup2(err, STDERR_FILENO);
			}	
            if (out != STDOUT_FILENO)
            {
                dup2(out, STDOUT_FILENO);
                if (close(out) < 0)
				{
					perror("close out");
				}
			}
            execvp(token[0], token);
            fprintf(stderr, "Unknown command: [%s].\n", token[0]);
            exit(1);
        }
        else
        {
            if (last_process)
            {
                last_process_pid = pid;
            }

            if (!number_pipe && last_process)
            {
                int status;
                waitpid(last_process_pid, &status, 0);
            }

            memset(token, 0, sizeof(char*) * TOKEN_MAX);
            
            arg = 0;
            next = 0;
            in  = STDIN_FILENO;
            out = clientfd;
            err = clientfd;

            if (pipe_table[process_count][0])
            {
                close(pipe_table[process_count][0]);
                pipe_table[process_count][0] = 0;
            }
            if (pipe_table[process_count][1])
            {
                close(pipe_table[process_count][1]);
                pipe_table[process_count][1] = 0;
            }
            if (cross_line_pipe_table[*cnt_line][0])
            {
                close(cross_line_pipe_table[*cnt_line][0]);
                cross_line_pipe_table[*cnt_line][0] = 0;
            }
            if (cross_line_pipe_table[*cnt_line][1])
            {
                close(cross_line_pipe_table[*cnt_line][1]);
                cross_line_pipe_table[*cnt_line][1] = 0;
            }

			if (from_id)
			{
				if (user_pipe[from_id][client_ptr->id][0] != 0)
				{
					close(user_pipe[from_id][client_ptr->id][0]);
					user_pipe[from_id][client_ptr->id][0] = 0;
				}
				if (user_pipe[from_id][client_ptr->id][1])
				{
					close(user_pipe[from_id][client_ptr->id][1]);
					user_pipe[from_id][client_ptr->id][1] = 0;
				}
			}
			if (to_id)
			{
				if (user_pipe[client_ptr->id][to_id][1])
				{
					close(user_pipe[client_ptr->id][to_id][1]);
					user_pipe[client_ptr->id][to_id][1] = 0;
				}
			}
        }
    } 
    free(token);
}

void my_shell(int clientfd, pid_t client_pid) {

    char del = ' ';
	int n;
	client *client_ptr = get_client_by_pid(client_pid);

    setenv("PATH", "bin:.", 1);
    
	while (1)
	{
		send(clientfd, "% ", 2, 0);

		string buff;
		vector<string> command;
		if ((n = readline(clientfd, buff)) <= 0)
		{
			if (n == 0)
			{
				// didn't receieve the message.
				continue;
			}
			else
			{
				break;
			}
		}
		split_command(command, buff, del);
		client_ptr->cnt_line = (client_ptr->cnt_line + 1) % FILE_MAX;

		if (command[0] == "setenv" || command[0] == "printenv")
		{
			env_handler(command, clientfd);
		}
		else if (command[0] == "exit")
		{
			// TODO: clean up all user_pipe about clientfd
			broadcast(client_pid, LOGOUT, NULL, -1, -1);
			break;
		}
		else if (command[0] == "who")
		{
			handle_who(client_ptr);
		}
		else if (command[0] == "tell")
		{
			int to_id = atoi(strdup(command[1].c_str()));
			char *tell_message;
			strtok_r(strdup(buff.c_str()), " ", &tell_message);
			strtok_r(NULL, " ", &tell_message);
			handle_tell(client_ptr, to_id, tell_message);
		}
		else if (command[0] == "yell")
		{
			char *yell_message;
			strtok_r(strdup(buff.c_str()), " ", &yell_message);
			broadcast(client_pid, YELL, yell_message, -1, -1);
		}
		else if (command[0] == "name")
		{
			char *new_name;
			strtok_r(strdup(buff.c_str()), " ", &new_name);
			if (checkname(client_ptr->pid, new_name) > 0)
			{
				memset(client_ptr->name, 0, sizeof(client_ptr->name));
				strncpy(client_ptr->name, new_name, strlen(new_name));
				broadcast(client_pid, CHANGENAME, new_name, -1, -1);
			}
			else
			{
				char temp[BUFFER_SIZE] = {0};
				sprintf(temp, "*** User '%s' already exists. ***\n", new_name);
				send(clientfd, temp, strlen(temp), 0);
			}
		}
		else if (check_user_pipe(command))
		{
			// handle user_pipe
			int from_id = 0, to_id = 0;
			bool receive_pipe = false;
			bool sendto_pipe = false;
			char temp[BUFFER_SIZE] = {0};

			for (vector<string>::iterator it = command.begin(); it != command.end(); ++it)
			{
				char *p = strdup((*it).c_str());
				if ( (p[0] == '>' || p[0] == '<') && (strlen(p) >= 2) )
				{
					if (p[0] == '>')
					{
						sendto_pipe = true;
						to_id = atoi(strdup((*it).substr(1, strlen(p) - 1).c_str()));
					}
					else if (p[0] == '<')
					{
						receive_pipe = true;
						from_id = atoi(strdup((*it).substr(1, strlen(p) - 1).c_str()));
					}
				}
			} 

			if (receive_pipe)
			{
				if (get_client_by_id(from_id) == NULL)
				{
					// The client doesn't exist.
					memset(temp, 0, sizeof(temp));
					sprintf(temp, "*** Error: user #%d does not exist yet. ***\n", from_id);
					send(clientfd, temp, strlen(temp), 0);
					continue;
				}
				else if (user_pipe[from_id][client_ptr->id][0] == 0)
				{
					// pipe doesn't exist.
					memset(temp, 0, sizeof(temp));
					sprintf(temp, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", from_id, client_ptr->id);
					send(clientfd, temp, strlen(temp), 0);
					continue;
				}
				else
				{
					char *_temp = (char*) malloc(sizeof(char) * BUFFER_SIZE);
					sprintf(_temp, "%s", strdup(buff.c_str()));
					broadcast(client_pid, RECEIVEPIPE, _temp, from_id, client_ptr->id);
					free(_temp);
				}
			}
			if (sendto_pipe)
			{
				if (get_client_by_id(to_id) == NULL)
				{
					// The client does not exist.
					memset(temp, 0, sizeof(temp));
					sprintf(temp, "*** Error: user #%d does not exist yet. ***\n", to_id);
					send(clientfd, temp, strlen(temp), 0);
					continue;
				}
				else if (user_pipe[client_ptr->id][to_id][0] || user_pipe[client_ptr->id][to_id][1])
				{
					memset(temp, 0, sizeof(temp));
					sprintf(temp, "*** Error: the pipe #%d->#%d already exists. ***\n", client_ptr->id, to_id);
					send(clientfd, temp, strlen(temp), 0);
					continue;
				}
				else
				{
					broadcast(client_pid, SENDPIPE, strdup(buff.c_str()), -1, to_id);
				}
			}

			process_handler(buff, &(client_ptr->cnt_line), client_ptr->cross_line_pipe_table, client_pid, true);

			if (to_id)
			{
				if (user_pipe[client_ptr->id][to_id][1] && (user_pipe[client_ptr->id][to_id][1] != clientfd))
				{
					close(user_pipe[client_ptr->id][to_id][1]);
					user_pipe[client_ptr->id][to_id][1] = 0;
				}
			}
			if (from_id)
			{
				if (user_pipe[from_id][client_ptr->id][0] && (user_pipe[from_id][client_ptr->id][0] != clientfd))
				{
					close(user_pipe[from_id][client_ptr->id][0]);
					user_pipe[from_id][client_ptr->id][0] = 0;
				}
				if (user_pipe[from_id][client_ptr->id][1] && (user_pipe[from_id][client_ptr->id][1] != clientfd))
				{
					close(user_pipe[from_id][client_ptr->id][1]);
					user_pipe[from_id][client_ptr->id][1] = 0;
				}
			}
		}
		else
		{
			process_handler(buff, &(client_ptr->cnt_line), client_ptr->cross_line_pipe_table, client_pid, false);
		}
	} // end of while.

}

void split_command(vector<string> &command, string input, char del) 
{
    stringstream ss(input);
    string item;
    // Make input as a standard input stream.
    while (getline(ss, item, del))
    {
        command.push_back(item);
    }
}

static void env_handler(vector<string> command, int clientfd)
{
    char env_var[100] = {0};

    if (command[0] == "setenv")
    {
        // memset(client_ptr->path, 0, sizeof(client_ptr->path));
		setenv(strdup(command[1].c_str()), strdup(command[2].c_str()), 1);
    }
    else
    {
        strcpy(env_var, getenv(command[1].c_str()));
		strcat(env_var, "\n");
		write(clientfd, env_var, strlen(env_var));
    }
}

static void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void clean_zombie()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa , NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

static void handle_who(client *client_ptr)
{
    int fd;
	memset(share_message, 0, sizeof(share_message));

    sprintf(share_message, "<ID>\t<nickname>\t<IP/port>\t<indicate me>\n");
    for (int i = 1; i <= CLIENT_NUMBER; ++i)
    {
		if (client_info[i].enable)
		{
			if (client_info[i].pid == client_ptr->pid)
			{
				char temp[500] = {0};
				sprintf(temp, "%d\t%s\tCGILAB/511\t<-me\n", client_info[i].id, client_info[i].name);                
				strncat(share_message, temp, strlen(temp));

			}
			else
			{
				char temp[500] = {0};
				sprintf(temp, "%d\t%s\tCGILAB/511\t\n", client_info[i].id, client_info[i].name);
				strncat(share_message, temp, strlen(temp));
			}
        }
    }
	kill(client_ptr->pid, SIGUSR1);
}

static void handle_tell(client *sender, int to_id, char *message)
{
    bool found = false;
    char _message[500] = {0};
    int receiver;
    client *rec = get_client_by_id(to_id);

    if (rec != NULL && rec->enable > 0)
    {
        found = true;
        receiver = rec->client_fd;
    }

    if (found)
    {
		memset(share_message, 0, sizeof(char) * BUFFER_SIZE);
        sprintf(share_message, "*** %s told you ***: %s\n", sender->name, message);
		kill(rec->pid, SIGUSR1);
		sleep(0.5);
    }
    else
    {
        sprintf(_message, "*** Error: user #%d does not exist yet. ***\n", to_id);
        send(sender->client_fd, _message, strlen(_message), 0);
    }
}

static bool check_user_pipe(vector<string> command)
{
	for (vector<string>::iterator it = command.begin(); it != command.end(); ++it)
	{
		char *p = strdup((*it).c_str());
		if ((p[0] == '>' || p[0] == '<') && p[1] != '\0')
		{
			return true;
		}
	}
	return false;
}

void recv_broadcast()
{
	struct sigaction sa;
	sa.sa_handler = sigusr1_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sa, NULL) == -1)
	{
		perror("sigaction sigusr1");
	}
}

void sigusr1_handler(int s)
{
	for (int i = 1; i <= CLIENT_NUMBER; ++i)
	{
		if (client_info[i].enable)
		{
			send(client_info[i].client_fd, share_message, strlen(share_message), 0);
			break;
		}
	}
}
