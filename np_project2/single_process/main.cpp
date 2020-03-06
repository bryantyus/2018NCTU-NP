#include "shell.h"
#include "socket.h"

client client_info[CLIENT_NUMBER];
// Declare the client information

extern int user_pipe[CLIENT_NUMBER + 1][CLIENT_NUMBER + 1][2];

static void send_welcome(int);

char *PORT;

int main(int argc, char *argv[]) {

  string command;
  int sockfd, clientfd, fdmax;
  int client_cnt = 0;
  fd_set rfd, afd;

  if (argc < 2) {
    printf("Usage: ./np_single_proc <port>\n");
    exit(0);
  }

  PORT = argv[1];

  socket_connect(&sockfd);
  clean_zombie();

  FD_ZERO(&afd);
  FD_ZERO(&rfd);
  FD_SET(sockfd, &afd);

  fdmax = sockfd;
  memset(user_pipe, 0, sizeof(user_pipe));
  // initialize the user pipe.

  while (1) {
    memcpy(&rfd, &afd, sizeof(afd));

    while (select(fdmax + 1, &rfd, NULL, NULL, NULL) == -1) {
      ;
    }

    for (int fd = 0; fd <= fdmax; ++fd) {
      if (FD_ISSET(fd, &rfd)) {
        if (fd == sockfd) {
          // accept client.
          clientfd = connect_client(sockfd);
          client_cnt++;
          if (client_cnt <= 30) {
            if (clientfd > fdmax) {
              fdmax = clientfd;
            }
            FD_SET(clientfd, &afd);
            init_client(clientfd);
            send_welcome(clientfd);
            broadcast(clientfd, LOGIN, NULL, -1, -1);
            send(clientfd, "% ", 2, 0);
          } else {
            client_cnt--;
          }
        } else {
          // handle client.
          if (my_shell(fd) < 0) {
            broadcast(fd, LOGOUT, NULL, -1, -1);
            FD_CLR(fd, &afd);
            reset_client(fd);
            close(fd);
            client_cnt--;
          }
        }
      }
    }
  }
}

static void send_welcome(int clientfd) {
  char welcome[300] = "****************************************\n"
                      "** Welcome to the information server. **\n"
                      "****************************************\n";

  send(clientfd, welcome, strlen(welcome), 0);
}
