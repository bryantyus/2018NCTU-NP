#include "socket.h"
#include "shell.h"

// #define PORT "4567"
#define BACKLOG 30

extern char *PORT;

static void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	else
	{
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
	}
}

void socket_connect(int *sockfd)
{
	struct addrinfo hints, *servinfo, *p;
	int rv, yes = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server : socket");
			continue;
		}

		if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		
		if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("server : bind");
			close(*sockfd);
			continue;
		}
		
		break;
	}
	
	if (p == NULL)
	{
		fprintf(stderr, "server : failed to bind\n");
		exit(1);
	}
	
	freeaddrinfo(servinfo);

	if (listen(*sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}
}


int readline(int fd, string &command)
{
	int buffersize = BUFFER_SIZE;
	int n, position = 0;
	char c;
	char *buffer = (char *) malloc(sizeof(char) * buffersize);
	if (!buffer)
	{
		fprintf(stderr, "npserver: allocation err\n");
		exit(1);
	}

	while (1)
	{
		n = read(fd, &c, 1);

		if (c == '\n' || c == '\r')
		{
			buffer[position] = '\0';
			command.assign(buffer, position);
			free(buffer);
			return position;
		}
		else if (n == -1)
		{	
			perror("read");
			free(buffer);
			return -1;
		}
		else
		{
			buffer[position++] = c;
		}

		if (position >= buffersize)
		{
			buffersize += BUFFER_SIZE;
			buffer = (char *) realloc(buffer, buffersize);
			if (!buffer)
			{
				fprintf(stderr, "npserver: allocation err\n");
				exit(1);
			}
		}
	}
}

int	connect_client(int sockfd)
{
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];
	int clientfd;

	sin_size = sizeof(their_addr);
	clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	
	return clientfd;	
}	
