#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

using namespace std;

unsigned short PORT;

typedef struct {
  unsigned short dst_port;
  unsigned short source_port;
  uint32_t dst_ip;
  uint32_t source_ip;
  uint8_t cd_value;
} socks_server_info;

typedef struct {
  string S_IP;
  string D_IP;
  unsigned short S_PORT;
  unsigned short D_PORT;
  string command;
  string reply;
} socks_record_info;

socks_record_info socks_record;
socks_server_info socks_server;

static void print_verbose();
static void clean_zombie();
static void sigchld_handler(int);
static void client_handler(int);
static void resolve_request(uint8_t *);
static void generate_reply(uint8_t *, uint8_t *);
static void set_server_sockfd(int *);
static void do_transfer(int, int);
static int do_bindmode(int, uint8_t *);
static int fire_wall(uint8_t *);
static int tcp_connect();
static int get_client_fd(int);

static void sigsegv_handler(int s) { system("pkill socks_server"); }

int main(int argc, char *argv[]) {
  int sockfd, clientfd, optval = 1;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./http_server <port>");
    return 0;
  }

  PORT = (unsigned short)atoi(argv[1]);

  set_server_sockfd(&sockfd);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  clean_zombie();

  while (1) {
    clientfd = get_client_fd(sockfd);
    if (clientfd > 0) {
      switch (fork()) {
      case -1:
        perror("fork");
        break;
      case 0:
        close(sockfd);
        client_handler(clientfd);
        close(clientfd);
        break;
      default:
        close(clientfd);
        break;
      }
    }
  }
  close(sockfd);

  return 0;
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

static void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

static void client_handler(int browser_socket) {
  int dst_socket, n, bind_fd;
  char buffer[BUFFER_SIZE] = {0};
  uint8_t socks4_reply[8] = {0};
  uint8_t socks4_request[BUFFER_SIZE] = {0};

  memset(&socks_server, 0, sizeof(socks_server));

  if ((n = read(browser_socket, socks4_request, BUFFER_SIZE)) < 0) {
    perror("read");
    exit(1);
  }

  if (socks4_request[0] != 0x04) {
    fprintf(stderr, "Usage: socks4\n");
    exit(0);
  }
  // unpacking the package buffer.
  resolve_request(socks4_request);
  generate_reply(socks4_reply, socks4_request);

  if (socks4_request[1] == 0x01) {
    socks_record.command = "CONNECT";
    // connect mode.
    if ((dst_socket = tcp_connect()) < 0 || !fire_wall(socks4_request)) {
      socks_record.reply = "Reject";
      print_verbose();
      socks4_reply[1] = 0x5B;
      write(browser_socket, socks4_reply, 8);
      close(dst_socket);
      close(browser_socket);
      exit(0);
    } else {
      socks_record.reply = "Accept";
      print_verbose();
      socks4_reply[1] = 0x5A;
      write(browser_socket, socks4_reply, 8);
    }
  } else if (socks4_request[1] == 0x02) {
    socks_record.command = "BIND";
    // bind mode.
    if (!fire_wall(socks4_request)) {
      socks_record.reply = "Reject";
      print_verbose();
      socks4_reply[1] = 0x5B;
      close(browser_socket);
      exit(0);
    }

    else {
      socks_record.reply = "Accept";
      print_verbose();
      socks4_reply[0] = 0x00;
      socks4_reply[1] = 0x5A;
      dst_socket = do_bindmode(browser_socket, socks4_reply);
    }
  }

  // establishment success.
  // generate the reply package back to browser .
  do_transfer(browser_socket, dst_socket);
}

static void resolve_request(uint8_t *request) {
  string _dst_ip = "";
  socks_server.cd_value = request[1];
  socks_server.dst_port = request[2] * 256 + request[3];

  for (int i = 4; i < 8; ++i) {
    _dst_ip.append(to_string(request[i]));
    if (i != 7) {
      _dst_ip.append(".");
    }
  }
  socks_record.D_IP = _dst_ip;
  socks_record.D_PORT = socks_server.dst_port;

  socks_server.dst_ip =
      request[7] << 24 | request[6] << 16 | request[5] << 8 | request[4];
}

static void generate_reply(uint8_t *reply, uint8_t *request) {
  memcpy(reply, request, 8);
  reply[0] = 0;
}

static void set_server_sockfd(int *listen_fd) {
  int client_fd, rv;
  int yes = 1;
  struct addrinfo hints, *res, *p;
  char port[20] = {0};

  sprintf(port, "%hu", PORT);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, port, &hints, &res)) != 0) {
    fprintf(stderr, "selectserver :%s\n", gai_strerror(rv));
    exit(EXIT_FAILURE);
  }

  for (p = res; p; p = p->ai_next) {
    if ((*listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) <
        0) {
      continue;
    }

    setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(*listen_fd, p->ai_addr, p->ai_addrlen) < 0) {
      close(*listen_fd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "selectserver : failed to bind\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(res);

  if (listen(*listen_fd, 10) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

static int get_client_fd(int sockfd) {
  // struct sockaddr_storage remoteaddr;
  struct sockaddr_in remote_addr;
  int client_fd;
  socklen_t addrlen;
  char _s_ip[100] = {0};

  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;

  addrlen = sizeof(remote_addr);
  client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &addrlen);

  inet_ntop(AF_INET, &remote_addr.sin_addr.s_addr, _s_ip, sizeof(remote_addr));

  socks_record.S_IP = string(_s_ip);
  socks_record.S_PORT = remote_addr.sin_port;

  if (client_fd == -1) {
    return -1;
  } else {
    return client_fd;
  }
}

static int tcp_connect() {
  int dst_socket, n;
  struct sockaddr_in serv;

  dst_socket = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv, 0, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = socks_server.dst_ip;
  serv.sin_port = htons(socks_server.dst_port);

  if (connect(dst_socket, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
    cout << "refuse connection" << endl;
    return -1;
  }
  return dst_socket;
}

static int fire_wall(uint8_t *req) {
  int permit = 0, i = 0, cnt = 0;
  char mode;
  char *save_ptr, *pch, *del = ".";
  char permit_ip[BUFFER_SIZE] = {0};
  char buffer[BUFFER_SIZE] = {0};
  uint8_t ip[4] = {0};
  FILE *fp;

  if ((fp = fopen("socks.conf", "r")) == NULL) {
    perror("fopen");
    exit(0);
  }

  while (fgets(buffer, BUFFER_SIZE, fp)) {
    memset(&i, 0, sizeof(i));
    memset(&cnt, 0, sizeof(cnt));
    sscanf(buffer, "permit %c %s", &mode, permit_ip);

    if (req[1] == 0x01) {
      // connect mode
      if (mode == 'b') {
        continue;
      }
    }

    else if (req[1] == 0x02) {
      // bind mode
      if (mode == 'c') {
        continue;
      }
    }

    pch = strtok_r(permit_ip, del, &save_ptr);

    while (pch) {
      if (!strncmp(pch, "*", 1))
        cnt++;

      ip[i++] = (uint8_t)atoi(pch);
      pch = strtok_r(NULL, del, &save_ptr);
    }

    // condition
    if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0 && cnt == 4) {
      permit = 1;
    }

    else if (ip[0] == req[4] && ip[1] == 0 && ip[2] == 0 && ip[3] == 0 &&
             cnt == 3) {
      permit = 1;
    }

    else if (ip[0] == req[4] && ip[1] == req[5] && ip[2] == 0 && ip[3] == 0 &&
             cnt == 2) {
      permit = 1;
    }

    else if (ip[0] == req[4] && ip[1] == req[5] && ip[2] == req[6] &&
             ip[3] == 0 && cnt == 1) {
      permit = 1;
    }

    else if (ip[0] == req[4] && ip[1] == req[5] && ip[2] == req[6] &&
             ip[3] == req[7] && cnt == 0) {
      permit = 1;
    }

    memset(buffer, 0, sizeof(buffer));
    memset(permit_ip, 0, sizeof(permit_ip));
    memset(ip, 0, sizeof(ip));
  }

  return permit;
}

static void do_transfer(int source_socket, int dest_socket) {
  int len, fdmax, i, n;
  char buffer[BUFFER_SIZE] = {0};
  fd_set rfd, afd;

  fdmax = (source_socket > dest_socket) ? source_socket : dest_socket;

  FD_ZERO(&rfd);
  FD_ZERO(&afd);
  FD_SET(dest_socket, &afd);
  FD_SET(source_socket, &afd);

  while (1) {
    rfd = afd;

    if ((n = select(fdmax + 1, &rfd, NULL, NULL, NULL)) < 0) {
      if (errno == EBADF) {
        exit(0);
      } else {
        continue;
      }
    }

    for (i = 1; i <= fdmax; i++) {
      memset(buffer, 0, sizeof(buffer));

      if (i == source_socket && FD_ISSET(i, &rfd)) {
        // read from browser , write into web.
        len = read(i, buffer, sizeof(buffer));

        if (!len) {
          FD_CLR(i, &afd);
          close(source_socket);
          close(dest_socket);
          break;
        } else {
          // printf("upload : %s\n",buffer);
          write(dest_socket, buffer, len);
        }
      }

      else if (i == dest_socket && FD_ISSET(i, &rfd)) {
        len = read(i, buffer, sizeof(buffer));

        if (!len) {
          FD_CLR(i, &afd);
          close(source_socket);
          close(dest_socket);
          break;
        }

        else {
          write(source_socket, buffer, len);
        }
      }
    }
  }
}

static void print_verbose() {
  printf("-----------------------\n");
  printf("<S_IP>: %s\n", socks_record.S_IP.data());
  printf("<S_PORT>: %hu\n", socks_record.S_PORT);
  printf("<D_IP>: %s\n", socks_record.D_IP.data());
  printf("<D_PORT>: %hu\n", socks_record.D_PORT);
  printf("<Command>: %s\n", socks_record.command.data());
  printf("<Reply>: %s\n", socks_record.reply.data());
  printf("-----------------------\n");
}

static int do_bindmode(int source_socket, uint8_t *socks4_reply) {
  int dest_socket, addrlen, bind_socket;
  struct sockaddr_in dest_addr, sa;
  socklen_t len;

  memset(&dest_addr, 0, sizeof(dest_addr));

  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  dest_addr.sin_port = htons(INADDR_ANY);

  addrlen = sizeof(dest_addr);
  len = sizeof(dest_addr);

  bind_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (bind(bind_socket, (struct sockaddr *)&dest_addr, addrlen) < 0) {
    perror("bind");
    return -1;
  }

  if (getsockname(bind_socket, (struct sockaddr *)&sa, (socklen_t *)&len) < 0) {
    perror("getsockname");
    return -1;
  }

  if (listen(bind_socket, 10) < 0) {
    perror("listen");
    return -1;
  }

  socks4_reply[1] = 0x5A;
  socks4_reply[2] = (uint8_t)(ntohs(sa.sin_port) / 256);
  socks4_reply[3] = (uint8_t)(ntohs(sa.sin_port) % 256);
  for (int i = 4; i < 8; ++i)
    socks4_reply[i] = 0x00;

  memset(&sa, 0, sizeof(sa));

  write(source_socket, socks4_reply, 8);

  if ((dest_socket = accept(bind_socket, (struct sockaddr *)&sa, &len)) < 0) {
    perror("accept");
    return -1;
  }
  write(source_socket, socks4_reply, 8);
  // send again.

  return dest_socket;
}
