#include "shell.h"
#include "socket.h"

char *PORT;

int main(int argc, char *argv[]) {

  string command;
  pid_t pid;
  int status, sockfd, clientfd;

  if (argc < 2) {
    printf("usage: ./npserver <port>\n");
    exit(0);
  }
  PORT = argv[1];
  socket_connect(&sockfd);
  clean_zombie();

  while (1) {
    clientfd = connect_client(sockfd);
    if (clientfd < 0) {
      perror("accept");
      continue;
    }
    switch ((pid = fork())) {
    case -1:
      perror("fork");
      close(clientfd);
      break;
    case 0:
      close(sockfd);
      my_shell(clientfd);
      break;
    default:
      close(clientfd);
      wait(&status);
    }
  }
}
