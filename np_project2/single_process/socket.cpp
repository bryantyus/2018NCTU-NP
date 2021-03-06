#include "socket.h"
#include "shell.h"

#define BACKLOG 30

extern char *PORT;

extern client client_info[CLIENT_NUMBER];
extern int user_pipe[CLIENT_NUMBER + 1][CLIENT_NUMBER + 1][2];

static int get_available_client();

static void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  } else {
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
  }
}

void socket_connect(int *sockfd) {
  struct addrinfo hints, *servinfo, *p;
  int rv, yes = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(rv));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("server : socket");
      continue;
    }

    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("server : bind");
      close(*sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server : failed to bind\n");
    exit(1);
  }

  freeaddrinfo(servinfo);

  if (listen(*sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
}

int readline(int fd, string &command) {
  int buffersize = BUFFER_SIZE;
  int n, position = 0;
  char c;
  char *buffer = (char *)malloc(sizeof(char) * buffersize);
  if (!buffer) {
    fprintf(stderr, "npserver: allocation err\n");
    exit(1);
  }

  while (1) {
    n = read(fd, &c, 1);

    if (c == '\n' || c == '\r') {
      buffer[position] = '\0';
      command.assign(buffer, position);
      free(buffer);
      return position;
    } else if (n == -1) {
      perror("read");
      free(buffer);
      return -1;
    } else {
      buffer[position++] = c;
    }

    if (position >= buffersize) {
      buffersize += BUFFER_SIZE;
      buffer = (char *)realloc(buffer, buffersize);
      if (!buffer) {
        fprintf(stderr, "npserver: allocation err\n");
        exit(1);
      }
    }
  }
}

int connect_client(int sockfd) {
  socklen_t sin_size;
  struct sockaddr_storage their_addr;
  char s[INET6_ADDRSTRLEN];
  int clientfd;

  sin_size = sizeof(their_addr);
  clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

  return clientfd;
}

void init_client(int clientfd) {
  struct sockaddr_in addr;
  socklen_t len;
  int client_index;

  len = sizeof(addr);
  if (getpeername(clientfd, (struct sockaddr *)&addr, &len) < 0) {
    perror("getpeername");
  }

  client_index = get_available_client();

  memset(&client_info[client_index], 0, sizeof(client_info[client_index]));
  sprintf(client_info[client_index].name, "(no name)");
  sprintf(client_info[client_index].path, "bin:.");

  inet_ntop(AF_INET, &(addr.sin_addr), client_info[client_index].ip_address,
            INET_ADDRSTRLEN);
  client_info[client_index].client_fd = clientfd;
  client_info[client_index].cnt_line = 0;
  client_info[client_index].id = client_index;
  client_info[client_index].port = (uint16_t)ntohs(addr.sin_port);
}

void broadcast(int clientfd, int option, char *_message, int from_id,
               int to_id) {
  client *client_ptr = get_client(clientfd);
  client *from_clinet = get_client_by_id(from_id);
  client *send_client = get_client_by_id(to_id);

  char message[BUFFER_SIZE] = {0};

  switch (option) {
  case LOGIN:
    sprintf(message, "*** User '%s' entered from CGILAB/511. ***\n",
            client_ptr->name);
    break;
  case LOGOUT:
    sprintf(message, "*** User '%s' left. ***\n", client_ptr->name);
    break;
  case YELL:
    sprintf(message, "*** %s yelled ***: %s\n", client_ptr->name, _message);
    break;
  case CHANGENAME:
    sprintf(message, "*** User from CGILAB/511 is named '%s'. ***\n", _message);
    break;
  case RECEIVEPIPE:
    sprintf(message, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n",
            client_ptr->name, client_ptr->id, from_clinet->name,
            from_clinet->id, _message);
    break;
  case SENDPIPE:
    sprintf(message, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n",
            client_ptr->name, client_ptr->id, _message, send_client->name,
            send_client->id);
    break;
  }

  for (int i = 1; i <= CLIENT_NUMBER; ++i) {
    if (client_info[i].client_fd) {
      send(client_info[i].client_fd, message, strlen(message), 0);
    }
  }
}

client *get_client(int clientfd) {
  for (int i = 1; i <= CLIENT_NUMBER; ++i) {
    if (client_info[i].client_fd == clientfd) {
      return &client_info[i];
    }
  }
  return NULL;
}

void reset_client(int clientfd) {
  client *client_ptr = get_client(clientfd);
  int client_id = client_ptr->id;

  memset(user_pipe[client_id], 0, sizeof(user_pipe[client_id]));
  for (int i = 1; i <= CLIENT_NUMBER; ++i) {
    memset(user_pipe[i][client_id], 0, sizeof(user_pipe[i][client_id]));
  }

  memset(client_ptr, 0, sizeof(client));
  close(clientfd);
}

static int get_available_client() {
  for (int target = 1; target <= CLIENT_NUMBER; ++target) {
    if (client_info[target].client_fd == 0) {
      return target;
    }
  }
  return -1;
}

client *get_client_by_id(int target_id) {
  client *ptr = NULL;
  for (int i = 1; i <= CLIENT_NUMBER; ++i) {
    if (client_info[i].id == target_id) {
      ptr = &client_info[i];
    }
  }
  return ptr;
}
