#include "shell.h"
#include "socket.h"


#define _CLIENT "/shm_client"
#define _MESSAGE "/shm_message"
#define _USER_PIPE "/shm_user_pipe"
#define _LOGCNT "/shm_logcnt"


// client client_info[CLIENT_NUMBER];
// Declare the client information

/*******************************************/

int (*user_pipe)[CLIENT_NUMBER + 1][2];
char *share_message;
client *client_info;
int *login_cnt;

// Reserved for shared memory
/********************************************/
char *PORT;

static void send_welcome(int);

int shm_client_info_fd;
int shm_message_fd;
int shm_userpipe_fd;
int shm_login_fd;

int main(int argc, char *argv[]) {

    string command;
    int sockfd, clientfd;
	int flag = O_RDWR | O_CREAT;
	pid_t pid;

	if (argc < 2)
	{
		printf("Usage: ./npserver <port>\n");
		exit(0);
	}

	PORT = argv[1];

	if (mkdir("./user_pipe", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0)
	{
		if (errno != EEXIST)
		{
			perror("mkdir");
		}
	}

	clean_zombie();

	shm_unlink(_LOGCNT);
	shm_unlink(_CLIENT);
	shm_unlink(_MESSAGE);
	shm_unlink(_USER_PIPE);

	shm_client_info_fd = shm_open(_CLIENT, flag, 0666);
	shm_message_fd = shm_open(_MESSAGE, flag, 0666);
	shm_userpipe_fd = shm_open(_USER_PIPE, flag, 0666);
	shm_login_fd = shm_open(_LOGCNT, flag, 0666);

	ftruncate(shm_client_info_fd, sizeof(client) * (CLIENT_NUMBER + 1));
	ftruncate(shm_message_fd, sizeof(char) * BUFFER_SIZE);
	ftruncate(shm_userpipe_fd, sizeof(int) * (BUFFER_SIZE + 1) * (BUFFER_SIZE + 1) * 2);	
	ftruncate(shm_login_fd, sizeof(int));

	user_pipe = (int(*)[CLIENT_NUMBER + 1][2]) get_shared_memory(shm_userpipe_fd, sizeof(int) * (CLIENT_NUMBER + 1) * (CLIENT_NUMBER + 1) * 2);
	share_message = (char *) get_shared_memory(shm_message_fd, sizeof(char) * BUFFER_SIZE);
	client_info = (client *) get_shared_memory(shm_client_info_fd, sizeof(client) * (CLIENT_NUMBER + 1));
	login_cnt = (int *) get_shared_memory(shm_login_fd, sizeof(int));

	memset(user_pipe, 0, sizeof(int) * (CLIENT_NUMBER + 1) * (CLIENT_NUMBER + 1) * 2);
	memset(share_message, 0, sizeof(char) * BUFFER_SIZE);
	memset(client_info, 0, sizeof(client) * (CLIENT_NUMBER + 1));
	// define and initialize all shared-memory	
	*login_cnt = 0;

	recv_broadcast();
	socket_connect(&sockfd);

	while (1)
	{
		if ((clientfd = connect_client(sockfd)) <= 0)
		{
			continue;
		}
		*login_cnt++;
		if (*login_cnt <= CLIENT_NUMBER)
		{
			send_welcome(clientfd);
			// success
			pid = fork();
			if (pid < 0)
			{
				continue;
			}
			if (pid == 0)
			{
				close(sockfd);
				init_client(clientfd, getpid());
				broadcast(getpid(), LOGIN, NULL, -1, -1);
				my_shell(clientfd, getpid());
				reset_client(getpid());
				exit(0);
			}
			else
			{
				// parent.
				close(clientfd);
			}
		}
		else
		{
			login_cnt--;
		}
	}
	shm_unlink(_CLIENT);
	shm_unlink(_MESSAGE);
	shm_unlink(_USER_PIPE);
	shm_unlink(_LOGCNT);
	close(shm_client_info_fd);
	close(shm_message_fd);
	close(shm_userpipe_fd);
	close(shm_login_fd);

	return 0;
}


static void send_welcome(int clientfd)
{
	char welcome[300] = "****************************************\n"
						"** Welcome to the information server. **\n"
						"****************************************\n";
	
	send(clientfd, welcome, strlen(welcome), 0);
}
