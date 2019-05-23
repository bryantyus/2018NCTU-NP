#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/asio.hpp>
#include <memory>
#include <fstream>


using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::address;



boost::asio::io_service global_io_service;

static void clean_zombie();
static void sigchld_handler(int);

class echo_session: public enable_shared_from_this<echo_session>
{
	public:
		echo_session(tcp::socket socket)
		:	_socket(move(socket))
		{}
		
		void start_service()
		{
			// get remote address.
			char _remote_port[50] = {0};

			tcp::endpoint remote_enpt 	= _socket.remote_endpoint();
			tcp::endpoint local_enpt	= _socket.local_endpoint();

			boost::asio::ip::address remote_address = remote_enpt.address();
			remote_addr = strdup(remote_address.to_string().c_str());
			remote_port = remote_enpt.port();

			sprintf(_remote_port, "%hu", remote_port);

			boost::asio::ip::address local_address = local_enpt.address();
			server_addr = strdup(local_address.to_string().c_str());

			filter(server_addr);
			filter(remote_addr);
			filter(_remote_port);

			get_http_data();
		}

	
	private:

		void do_service()
		{
			auto self(shared_from_this());

			char status_code[50] = "HTTP/1.1 200 OK\n";

			clean_zombie();

			_socket.async_send(
				boost::asio::buffer(status_code, 50), [this, self] (const boost::system::error_code ec, size_t write_length)
				{
					int socketfd = _socket.native_handle();					
					if (!ec)
					{
						char *save_ptr = NULL;
						char execution_path[10] = "./";
						pid_t pid;
						global_io_service.notify_fork(boost::asio::io_service::fork_prepare);

						cgi_name = strtok_r(cgi_name, "/", &save_ptr);
						strncat(execution_path, cgi_name, strlen(cgi_name));


						if ((pid = fork()) == -1)
						{
							perror("fork");
							exit(1);
						}
						else if (pid == 0)
						{
							dup2(socketfd, STDOUT_FILENO);
							global_io_service.notify_fork(boost::asio::io_service::fork_child);
							_socket.close();
							
							execlp(execution_path, cgi_name, NULL);
							exit(1);
						}
						else
						{
							global_io_service.notify_fork(boost::asio::io_service::fork_parent);
							_socket.close();
							wait(NULL);
						}

					}
				}
			);

		}

		void set_env_var()
		{
			char _remote_port[50] = {0};
			sprintf(_remote_port, "%hu", remote_port);
			setenv("REQUEST_METHOD", request_method, 1);
			setenv("REQUEST_URI", request_uri, 1);
			setenv("QUERY_STRING", query_string, 1);
			setenv("SERVER_PROTOCOL", server_protocol, 1);
			setenv("HTTP_HOST", http_host, 1);
			setenv("SERVER_ADDR", server_addr, 1);
			setenv("SERVER_PORT", server_port, 1);
			setenv("REMOTE_ADDR", remote_addr, 1);
			setenv("REMOTE_PORT", _remote_port, 1);
		}

		void get_http_data()
		{
			auto self(shared_from_this());

			memset(message, 0, sizeof(message));
			_socket.async_read_some(
				boost::asio::buffer (message, max_length),
				[this, self] (const boost::system::error_code ec, size_t length)
				{
					if (!ec)
					{
						parse_http_header();	
						set_env_var();
						do_service();
					}
					else
					{
						// cout << "Get data error" << endl;
					}
				}
			);
		}

		void parse_http_header()
		{
			vector<string> header_info;
			string content, item;
			char buffer[1024] = {0};
			strncpy(buffer, message, strlen(message));
			content = string(buffer);
			stringstream ss(content);

			while (getline(ss, item))
			{
				header_info.push_back(item);
			}

			header_info.pop_back();

			for (auto it = header_info.begin(); it != header_info.end(); ++it)
			{

				char *header_data = strdup((*it).c_str());
				char *save_ptr 	= NULL;
				strtok_r(header_data, ": ", &save_ptr);
				// Used to parse all First word.

				if (it == header_info.begin())
				{
					char *temp_cgi = NULL;
					request_method = header_data;
					temp_cgi = strtok_r(save_ptr, " ", &server_protocol);
					cgi_name = strtok_r(temp_cgi, "?", &query_string);
					request_uri = cgi_name;					

					filter(request_uri);
					filter(request_method);
					filter(server_protocol);

					// 	Get request method:
					//		format : GET
					//	Get cgi_name:
					//		format : /XXX.cgi
					//	Get server_protocol: 
					//		format : HTTP/1.1

					// cout << "request method: " << request_method << endl;
					// cout << "cgi name: " << cgi_name << endl;
					// cout << "server_protocol: " << server_protocol << endl;
				}
				else if (!strncmp(header_data, "Host", strlen("Host")))
				{
					http_host = strtok_r(save_ptr, ":", &server_port);
					// server_port[strlen(server_port) - 1] = '\0';
					http_host = strtok(http_host, " ");

					filter(server_port);
					filter(http_host);

					// setenv("HTTP_HOST", http_host, 1);
					// setenv("SERVER_PORT", server_port, 1);
					//	Get http host:
					//		format : nplinux1.cs.nctu.edu.tw
					//	Get server_port
					//		format : 10010
					// cout << "http host : " << http_host << endl;
					// cout << "server port : " << server_port << endl;
					// cout << "server address: " << server_addr << endl;

				}
			}
		}

		void filter(char *var)
		{
			for (int i = 0; i < strlen(var); ++i)
			{
				if (var[i] == '\r')
				{
					var[i] = '\0';
				}
			}
		}

	private:
		enum { max_length = 4096 };
		tcp::socket _socket;
		char message[max_length] = {0};
		char *request_method;
		char *request_uri;
		char *query_string;
		char *server_protocol;
		char *http_host;
		char *server_addr;
		char *server_port;
		char *remote_addr;
		char *cgi_name;
		unsigned short remote_port;

};

class tcp_server
{
	public:
		tcp_server(short port)
		:	_socket(global_io_service),
			_acceptor(global_io_service),
			_endpoint(tcp::v4(), port)
			{
				_acceptor.open(_endpoint.protocol());
				_acceptor.set_option(tcp::acceptor::reuse_address(true));
				_acceptor.bind(_endpoint);
				_acceptor.listen();
				// cout << "listening success" << endl;
				do_accept();
			}

	private:

		

		void do_accept()
		{
			_acceptor.async_accept(_socket, [this](const boost::system::error_code ec)
			{
				if (!ec)
				{
					// success
					make_shared<echo_session>(move(_socket))->start_service();
					_socket.close();
					// do_accept();
				}
					// _socket.close();
					do_accept();
					// waiting for accept.
			});
		}


	private:
			tcp::socket _socket;
			tcp::acceptor _acceptor;
			tcp::endpoint _endpoint;

};



int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./http_server <port>");
		return 0;
	}

	unsigned short port = (unsigned short)atoi(argv[1]);


	tcp_server http_server(port);
	global_io_service.run();

	return 0;
}

void clean_zombie(){
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

static void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}
