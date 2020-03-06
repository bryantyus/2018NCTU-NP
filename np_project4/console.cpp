#include <arpa/inet.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_SIZE 5
#define ITEM_NUM 3

using namespace std;
using boost::asio::ip::address;
using boost::asio::ip::tcp;

boost::asio::io_service global_io_service;

typedef struct {
  int server_index;
  int enable;
  char domain_name[50];
  char port[50];
  char filename[50];
} server_info;

typedef struct {
  char host[50];
  char port[50];
  int enable;
} socks_info;

server_info server[6];
socks_info socks;
// server[0] set to -1
static void parse_query(char *);
static void output_shell(char *, server_info &);
static void print_html_web();
static void network_entity(char *);

static int connect_number;
// setting the total number of connection.

class tcp_client {
public:
  tcp_client()
      : _socket(global_io_service), _resolver(global_io_service),
        socks_resolver(global_io_service) {}

  void do_resolve(server_info &_server) {
    tcp::resolver::query query(socks.host, socks.port);

    _resolver.async_resolve(query,
                            [this, &_server](const boost::system::error_code ec,
                                             tcp::resolver::iterator it) {
                              if (!ec) {
                                do_connect(_server, it);
                              } else {
                                cout << "async_resolve err" << endl;
                              }
                            });
  }

  void do_connect(server_info &_server, tcp::resolver::iterator &it) {
    tcp::endpoint _endpoint = it->endpoint();
    _socket.async_connect(_endpoint, [this,
                                      &_server](boost::system::error_code ec) {
      if (!ec) {
        // connect success.
        uint8_t message[500] = {0};
        uint8_t reply[8] = {0};
        char *pch;
        char net_ip[50] = {0};
        tcp::resolver::query _query(_server.domain_name, _server.port);
        string ss =
            socks_resolver.resolve(_query)->endpoint().address().to_string();

        strncpy(net_ip, ss.c_str(), strlen(ss.c_str()));

        uint32_t port = (uint32_t)atoi(_server.port);
        message[0] = 0x04;
        message[1] = 0x01;
        message[2] = port / 256;
        message[3] = port % 256;

        pch = strtok(net_ip, ".");
        message[4] = (uint8_t)atoi(pch);

        for (int j = 1; j <= 3; j++) {
          pch = strtok(NULL, ".");
          message[j + 4] = (uint8_t)atoi(pch);
        }

        _socket.send(boost::asio::buffer(message, 4096));

        _socket.read_some(boost::asio::buffer(reply, 8));
        for (int i = 0; i < 8; ++i)
          printf("reply[%d]: %hu\n", i, reply[i]);

        if (reply[1] == 0x5A) {
          do_read(_server);
        }
      } else {
        // cout << "async_connect error" << endl;
      }
    });
    return;
  }

  void do_read(server_info &_server) {
    _socket.async_read_some(
        boost::asio::buffer(message, max_length),
        [this, &_server](const boost::system::error_code ec, size_t length) {
          if (!ec) {
            output_shell(message, _server);
            if (contain_prompt(message)) {
              memset(message, 0, sizeof(message));
              send_command(_server);
            }
            memset(message, 0, sizeof(message));
            do_read(_server);
          }
        });
    return;
  }

  /*
   * read file and write commands to server
   */
  void send_command(server_info &_server) {
    char buffer[4096] = {0};
    char *_temp;
    char temp[4096] = {0};
    string content;
    size_t n;

    if (!fin.getline(buffer, 4096)) {
      fin.close();
      return;
    }
    n = strlen(buffer);
    content = string(buffer);

    while (content.back() == '\r' || content.back() == '\n') {
      content.pop_back();
    }
    content.append("&NewLine;");
    _temp = strdup(content.c_str());
    strncpy(temp, _temp, strlen(_temp));
    network_entity(temp);

    printf("<script>document.getElementById(\'s%d\').innerHTML += "
           "\'<b>%s</b>\'; </script>\n",
           _server.server_index, temp);
    fflush(stdout);
    buffer[n] = '\n';
    do_write(buffer, n + 1);
  }

  /*
      write data to server.
  */
  void do_write(char *buffer, size_t length) {
    _socket.async_write_some(
        boost::asio::buffer(buffer, length),
        [&](const boost::system::error_code ec, size_t write_length) {});
    return;
  }

public:
  ifstream fin;

private:
  tcp::socket _socket;
  tcp::resolver _resolver;
  tcp::resolver socks_resolver;
  enum { max_length = 4096 };
  char message[max_length] = {0};

private:
  bool contain_prompt(char *_message) {
    for (int i = 0; i < max_length; ++i) {
      if (_message[i] == '%' && _message[i + 1] == ' ') {
        return true;
      }
    }
    return false;
  }
};

int main(void) {
  // setenv ("QUERY_STRING",
  // "h0=nplinux1.cs.nctu.edu.tw&p0=9999&f0=t1.txt&h1=nplinux1.cs.nctu.edu.tw&p1=9999&f1=t2.txt&h2=nplinux1.cs.nctu.edu.tw&p2=9999&f2=t3.txt&h3=nplinux1.cs.nctu.edu.tw&p3=9999&f3=t4.txt&h4=nplinux1.cs.nctu.edu.tw&p4=9999&f4=t5.txt&sh=nplinux5.cs.nctu.edu.tw&sp=4567",
  // 1);
  // setenv ("QUERY_STRING",
  // "h0=nplinux1.cs.nctu.edu.tw&p0=9999&f0=t1.txt&h1=nplinux1.cs.nctu.edu.tw&p1=9999&f1=t2.txt&h2=nplinux1.cs.nctu.edu.tw&p2=9999&f2=t3.txt&h3=nplinux1.cs.nctu.edu.tw&p3=9999&f3=t4.txt&h4=nplinux1.cs.nctu.edu.tw&p4=9999&f4=t5.txt&sh=nplinux5.cs.nctu.edu.tw&sp=4567",
  // 1); setenv ("QUERY_STRING",
  // "h0=nplinux4.cs.nctu.edu.tw&p0=10000&f0=t1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=",
  // 1); setenv ("QUERY_STRING",
  // "h0=nplinux1.cs.nctu.edu.tw&p0=10000&f0=t1.txt&h1=nplinux1.cs.nctu.edu.tw&p1=10000&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=",
  // 1); setenv ("QUERY_STRING",
  // "h0=nplinux4.cs.nctu.edu.tw&p0=10000&f0=t1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=nplinux4.cs.nctu.edu.tw&p4=10000&f4=t5.txt",
  // 1); setenv ("QUERY_STRING",
  // "h0=nplinux1.cs.nctu.edu.tw&p0=9999&f0=t1.txt&h1=nplinux1.cs.nctu.edu.tw&p1=9999&f1=t2.txt&h2=nplinux1.cs.nctu.edu.tw&p2=9999&f2=t3.txt&h3=&p3=&f3=&h4=&p4=&f4=&sh=nplinux5.cs.nctu.edu.tw&sp=4567",
  // 1);

  char *query_str = getenv("QUERY_STRING");
  int server_index = 1;

  parse_query(query_str);

  print_html_web();

  tcp_client client[6];
  try {
    for (int i = 1; i <= SERVER_SIZE; ++i) {
      if (server[i].enable) {
        server[i].server_index = server_index;
        chdir("test_case");
        client[i].fin.open(server[i].filename, std::ifstream::in);
        chdir("../");
        client[i].do_resolve(server[i]);
        server_index++;
      }
    }
  } catch (std::exception &e) {
    cerr << "Exception: " << e.what() << endl;
  }
  global_io_service.run();

  return 0;
}

static void parse_query(char *query) {
  int n;
  for (int i = 1; i <= SERVER_SIZE; ++i) {
    if (i == SERVER_SIZE) {
      char *pch;
      char temp[100] = {0};
      char *_temp = temp;
      strncpy(temp, query, strlen(query));

      for (int i = 1; i <= 3; ++i) {
        strtok_r(_temp, "&", &_temp);
      }
      sscanf(_temp, "sh=%[a-zA-Z0-9.]&sp=%[0-9]", socks.host, socks.port);
      // cout << "_temp: " << _temp << endl;

      n = sscanf(query,
                 "h%*[0-9]=%[a-zA-Z0-9.]&p%*[0-9]=%[0-9]&f%*[0-9]=%[a-zA-Z0-9.]"
                 "&sh=%[a-zA-Z0-9.]&sp=%[0-9]&",
                 server[i].domain_name, server[i].port, server[i].filename,
                 socks.host, socks.port);
      socks.enable = 1;
    } else {
      n = sscanf(
          query,
          "h%*[0-9]=%[a-zA-Z0-9.]&p%*[0-9]=%[0-9]&f%*[0-9]=%[a-zA-Z0-9.]&",
          server[i].domain_name, server[i].port, server[i].filename);
    }
    for (int j = 1; j <= ITEM_NUM; ++j) {
      strtok_r(query, "&", &query);
    }
    if (n == ITEM_NUM) {
      connect_number++;
      server[i].enable = 1;
    } else if (strlen(server[i].domain_name) != 0 &&
               strlen(server[i].port) != 0 && strlen(server[i].filename) != 0) {
      connect_number++;
      server[i].enable = 1;
    }
  }
}

static void print_html_web() {
  cout << "Content-type: text/html" << endl << endl;
  cout << "<meta charset=\"UTF-8\">";
  cout << "<title>NP Project 3 Console</title>";
  cout << "<link rel=\"stylesheet\" "
          "href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/"
          "bootstrap.min.css\" "
          "integrity=\"sha384-MCw98/"
          "SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\" "
          "crossorigin=\"anonymous\"/>";
  cout << "<link "
          "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\" "
          "rel=\"stylesheet\"/><link rel=\"icon\" type=\"image/png\" "
          "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/"
          "678068-terminal-512.png\"/>";

  cout << "<style>* { font-family: 'Source Code Pro', monospace; font-size: "
          "1rem !important}body {background-color: #212529;}pre {color: "
          "#cccccc;}b{color: #ffffff;}</style>";

  printf("</head><br>");
  printf("<body><br>");
  printf("<table class=\"table table-dark table-bordered\"><br>");
  printf("<thead><br>");
  printf("<tr><br>");

  for (int i = 1; i <= SERVER_SIZE; ++i) {
    if (server[i].enable) {
      printf("<th scope=\"col\">%s:%s</th><br>", server[i].domain_name,
             server[i].port);
    }
  }

  printf("</tr><br>");
  printf("</thead><br>");
  printf("<tbody><br>");
  printf("<tr><br>");

  int server_index = 1;
  for (int i = 1; i <= SERVER_SIZE; ++i) {
    if (server[i].enable) {
      printf("<td><pre id=\"s%d\" class=\"mb-0\"></pre></td><br>",
             server_index);
      server_index++;
    }
  }
  printf("</tr><br>");
  printf("</tbody><br>");
  printf("</table><br>");
  printf("</body><br>");
  printf("</html>\n");
}

static void output_shell(char *message, server_info &_server) {
  string content = string(message);
  stringstream ss(content);
  string item;
  char *_temp;
  char temp[4096] = {0};

  while (getline(ss, item, '\n')) {
    // cout << "item: " << item << endl;
    if (item != "% ")
      item.append("&NewLine;");

    _temp = strdup(item.c_str());
    strncpy(temp, _temp, strlen(_temp));
    network_entity(temp);
    printf("<script>document.getElementById(\'s%d\').innerHTML += "
           "\'%s\';</script>\n",
           _server.server_index, temp);
    fflush(stdout);
  }
}

static void network_entity(char *buffer) {
  char temp[4096] = {0};
  int k = 0, len = 0;
  char c;
  while ((c = buffer[k])) {
    switch (c) {
    case '<':
      strncat(temp, "&lt;", 4);
      len += 4;
      break;
    case '>':
      strncat(temp, "&gt;", 4);
      len += 4;
      break;
    case ' ':
      strncat(temp, "&nbsp;", 6);
      len += 6;
      break;
    case '\"':
      strncat(temp, "&quot;", strlen("&quot;"));
      len += strlen("&quot;");
      break;
    case '\'':
      strncat(temp, "&apos;", strlen("&apos;"));
      len += strlen("&apos;");
      break;
    case '\n':
      strncat(temp, "&NewLine;", strlen("&NewLine;"));
      len += strlen("&NewLine;");
      break;
    case '\r':
      strncat(temp, "&NewLine;", strlen("&NewLine;"));
      len += strlen("&NewLine;");
      break;
    default:
      temp[len++] = c;
      break;
    }
    k++;
  }
  temp[len] = '\0';
  memset(buffer, 0, sizeof(buffer));
  strncat(buffer, temp, strlen(temp));
}
