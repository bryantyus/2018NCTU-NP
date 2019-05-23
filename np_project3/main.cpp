#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/asio.hpp>
#include <memory>
#include <fstream>
#include <vector>
#include <direct.h>

#define SERVER_SIZE 5
#define ITEM_NUM 3

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::ip::address;

boost::asio::io_service global_io_service;
boost::asio::io_service io_service_for_client;

class echo_session;
class tcp_client;

typedef struct
{
	int server_index;
	int enable;
	char domain_name[50];
	char port[50];
	char filename[50];
} server_info;

server_info server[6];

static int connect_number;


class echo_session : public enable_shared_from_this<echo_session>
{
public:
	echo_session(tcp::socket socket)
		: _socket(move(socket))
	{}

	void start_service()
	{
		// get remote address.
		char _remote_port[50] = { 0 };

		/*
		tcp::endpoint remote_enpt = _socket.remote_endpoint();
		tcp::endpoint local_enpt = _socket.local_endpoint();

		boost::asio::ip::address remote_address = remote_enpt.address();
		remote_addr = _strdup(remote_address.to_string().c_str());
		remote_port = remote_enpt.port();

		sprintf_s(_remote_port, "%hu", remote_port);

		boost::asio::ip::address local_address = local_enpt.address();
		server_addr = _strdup(local_address.to_string().c_str());

		filter(server_addr);
		filter(remote_addr);
		filter(_remote_port);
		*/

		get_http_data();
	}

private:

	void send_cgi_web()
	{
		auto self(shared_from_this());

		string send_buffer = "";
		char s_buffer[10000] = {0};
		char temp[10000] = {0};

		sprintf_s(s_buffer, "Content-type: text/html\n\n");

		sprintf_s(temp, 
			"<!DOCTYPE html>"
			"<html lang = \"en\">"
			"<head>"
			"<title>NP Project 3 Panel</title>"
			"<link rel =\"stylesheet\" href =\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\" integrity =\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\" crossorigin =\"anonymous\"/>"
			"<link href =\"https://fonts.googleapis.com/css?family=Source+Code+Pro\" rel =\"stylesheet\"/>"
			"<link rel =\"icon\" type =\"image/png\" href =\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"/>"
			"<style>"
			"* {\n"
			"font - family: \'Source Code Pro\', monospace;"
			"}"
			"</style>"
			"</head>"
			"<body class = \"bg-secondary pt-5\">");

		strncat_s(s_buffer, temp, strlen(temp));
		
		memset(temp, 0, sizeof(temp));

		sprintf_s(temp, 
			"<form action =\"console.cgi\" method = \"GET\"><table class =\"table mx-auto bg-light\" style =\"width: inherit\"><thead class =\"thead-dark\"><tr>"
			"<th scope =\"col\">#</th><th scope =\"col\">Host</th><th scope =\"col\">Port</th><th scope =\"col\">Input File</th></tr></thead><tbody>");


		strncat_s(s_buffer, temp, strlen(temp));

		string test_file[10] = {"t1.txt", "t2.txt", "t3.txt", "t4.txt", "t5.txt", "t6.txt", "t7.txt", "t8.txt", "t9.txt", "t10.txt"};
		
		string final_test_menu = "";

		for (int i = 0; i < 10; ++i)
		{
			final_test_menu = final_test_menu + "<option value=\"" + test_file[i] + "\">" + test_file[i] + "</option>";
		}
		string domain_name = "cs.nctu.edu.tw";

		string host[10] = { "nplinux1", "nplinux2", "nplinux3", "nplinux4", "nplinux5", "npbsd1", "npbsd2", "npbsd3", "npbsd4", "npbsd5" };

		string host_menu[10];
		for (int i = 0; i < 9; i++)
		{
			host_menu[i] = "<option value=\"" + host[i] + "." + domain_name + "\">" + host[i] + "</option>";
		}

		string final_host_menu = "";
		for (int i = 0; i < 9; i++)
		{
			final_host_menu = final_host_menu + host_menu[i];
		}

		for (int i = 0; i < 5; ++i)
		{
			send_buffer += "<tr>";
			send_buffer = send_buffer + "<th scope =\"row\" class =\"align-middle\">Session" + std::to_string(i + 1) + "</th>";
			send_buffer += "<td>";
			send_buffer += "<div class =\"input-group\">";
			send_buffer = send_buffer + "<select name =\"h" + std::to_string(i) + "\" class =\"custom-select\">";
			send_buffer = send_buffer + final_host_menu + "</select>";
			send_buffer += "<div class =\"input-group-append\">";
			send_buffer += "<span class =\"input-group-text\">.cs.nctu.edu.tw</span>";
			send_buffer += "</div></div></td><td>";
			send_buffer = send_buffer + "<input name =\"p" + std::to_string(i) + "\" type =\"text\" class =\"form-control\" size =\"5\" />";
			send_buffer += "</td><td>";
			send_buffer = send_buffer + "<select name =\"f" + std::to_string(i) + "\" class =\"custom-select\">";
			send_buffer = send_buffer + final_test_menu + "</select></td></tr>";
		}

		memset(temp, 0, sizeof(temp));

		char *__temp = _strdup(send_buffer.c_str());
		strncat_s(s_buffer, __temp, strlen(__temp));

		sprintf_s(temp,
			"<tr><td colspan =\"3\"></td><td><button type=\"submit\" class = \"btn btn-info btn-block\">Run</button></td></tr></tbody></table></form></body></html>");

		strncat_s(s_buffer, temp, strlen(temp));

		_socket.async_send(
			boost::asio::buffer(s_buffer, strlen(s_buffer)), [self, this](const boost::system::error_code ec, size_t w_len)
			{
				if (!ec)
				{
					// cout << "success" << endl;
				}
				else
				{
					// cout << "async_send err: " << ec << endl;
				}
			}
		);

		//do_write(send_buffer);	
	}

	void do_write(char *_buffer)
	{
		_socket.async_send(
			boost::asio::buffer(_buffer, strlen(_buffer)), [this](const boost::system::error_code ec, size_t w_len)
			{
				if (!ec)
				{
				// success
				}
				else
				{
				// error
					cout << "async_write err : " << ec << endl;
				}
			}
		);
	}

	void do_service()
	{
		auto self(shared_from_this());
		char status_code[50] = "HTTP/1.1 200 OK\n";

		_socket.async_send(
			boost::asio::buffer(status_code, 50), [this, self](const boost::system::error_code ec, size_t write_length)
		{
			
			if (!ec)
			{
				if (!strncmp(cgi_name, "/panel.cgi", strlen("/panel.cgi")))
				{
					send_cgi_web();
				}
				else if (!strncmp(cgi_name, "/console.cgi", strlen("/console.cgi")))
				{
					int server_index = 1;
					
					//tcp_client client(io_service_for_client)[SERVER_SIZE + 1]; // initialize
					//tcp_client client[SERVER_SIZE + 1];
					//tcp_client client[SERVER_SIZE + 1];
					parse_query(query_string);
					print_html_web();
					tcp_client client[SERVER_SIZE + 1];

					try
					{
						
						for (int i = 1; i <= SERVER_SIZE; ++i)
						{
							if (server[i].enable)
							{
								server[i].server_index = server_index;
								server_index++;
								_chdir("test_case");
								client[i].fin.open(server[i].filename, std::ifstream::in);
								_chdir("../");
								client[i].get_start(server[i], *this);
							}
						}
						io_service_for_client.run();
					}
					catch (std::exception &e)
					{
						cerr << "Exception: " << e.what() << endl;
					}
				}
			}
			else
			{
				cout << "async_send err : " << ec << endl;
			}
		});
	}


	void parse_query(char *query)
	{
		int n;
		for (int i = 1; i <= SERVER_SIZE; ++i)
		{
			n = sscanf(query, "h%*[0-9]=%[a-zA-Z0-9.]&p%*[0-9]=%[0-9]&f%*[0-9]=%[a-zA-Z0-9.]&", server[i].domain_name, server[i].port, server[i].filename);
			for (int j = 1; j <= ITEM_NUM; ++j)
			{
				strtok_s(query, "&", &query);
			}
			if (n == ITEM_NUM)
			{
				connect_number++;
				server[i].enable = 1;
			}
		}

	}

	void print_html_web()
	{
		char temp[2048] = { 0 };
		char *__s_temp;
		char s_buffer[4096] = { 0 };
		string str = "";

		memset(temp, 0, sizeof(temp));

		sprintf_s(temp, "Content-type: text/html\n\n");
		str += "<meta charset=\"UTF-8\"><title>NP Project 3 Console</title><link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\" integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\" crossorigin=\"anonymous\"/>";
		str += "<link href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\" rel=\"stylesheet\"/><link rel=\"icon\" type=\"image/png\" href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"/>";
		str += "<style>* { font-family: 'Source Code Pro', monospace; font-size: 1rem !important}body {background-color: #212529;}pre {color: #cccccc;}b{color: #ffffff;}</style>";
		str += "</head><br><body><br><table class=\"table table-dark table-bordered\"><br><thead><br><tr><br>";

		__s_temp = _strdup(str.c_str());
		strncat(s_buffer, temp, strlen(temp));
		strncat(s_buffer, __s_temp, strlen(__s_temp));

		memset(temp, 0, sizeof(temp));

		for (int i = 1; i <= SERVER_SIZE; ++i)
		{
			if (server[i].enable)
			{
				sprintf_s(temp, "<th scope=\"col\">%s:%s</th><br>", server[i].domain_name, server[i].port);
				strncat(s_buffer, temp, strlen(temp));
				memset(temp, 0, sizeof(temp));
			}
		}

		sprintf_s(temp, "</tr><br></thead><br><tbody><br><tr><br>");
		strncat(s_buffer, temp, strlen(temp));
		memset(temp, 0, sizeof(temp));

		int server_index = 1;
		for (int i = 1; i <= SERVER_SIZE; ++i)
		{
			if (server[i].enable)
			{
				sprintf_s(temp, "<td><pre id=\"s%d\" class=\"mb-0\"></pre></td><br>", server_index);
				strncat(s_buffer, temp, strlen(temp));
				memset(temp, 0, sizeof(temp));
				server_index++;
			}
		}

		sprintf_s(temp, "</tr><br></tbody><br></table><br></body><br></html>\n");
		strncat(s_buffer, temp, strlen(temp));

		do_write(s_buffer);
	}

	/*
	void set_env_var()
	{
		char _remote_port[50] = { 0 };
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
	*/
	void get_http_data()
	{
		auto self(shared_from_this());
		memset(message, 0, sizeof(message));
		_socket.async_read_some(
			boost::asio::buffer(message, max_length),
			[this, self](const boost::system::error_code ec, size_t length)
		{
			if (!ec)
			{
				parse_http_header();
				// set_env_var();
				do_service();
			}
			else
			{
				// cout << "Get data error" << endl;
				cout << "asyc_read : err : " << ec << endl;
			}
		}
		);
	}

	void parse_http_header()
	{
		vector<string> header_info;
		string content, item;
		char buffer[1024] = { 0 };
		strncpy_s(buffer, message, strlen(message));
		content = string(buffer);
		stringstream ss(content);

		while (getline(ss, item))
		{
			header_info.push_back(item);
		}

		header_info.pop_back();

		for (auto it = header_info.begin(); it != header_info.end(); ++it)
		{

			char *header_data = _strdup((*it).c_str());
			char *save_ptr = NULL;
			strtok_s(header_data, ": ", &save_ptr);

			// Used to parse all First word.

			if (it == header_info.begin())
			{
				char *temp_cgi = NULL;
				request_method = header_data;
				temp_cgi = strtok_s(save_ptr, " ", &server_protocol);
				cgi_name = strtok_s(temp_cgi, "?", &query_string);
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

			}
			else if (!strncmp(header_data, "Host", strlen("Host")))
			{
				char *temp_ptr = NULL;
				http_host = strtok_s(save_ptr, ":", &server_port);
				// server_port[strlen(server_port) - 1] = '\0';
				http_host = strtok_s(http_host, " ", &temp_ptr);

				filter(server_port);
				filter(http_host);


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


	
public:
	friend class tcp_client; // declare that tcp_client can access members in echo_session.
	
private:
	enum { max_length = 4096 };
	tcp::socket _socket;
	char message[max_length] = { 0 };
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
	
	
class tcp_client
{
public:
	tcp_client()
		: _resolver(io_service_for_client),
		_socket(io_service_for_client)
	{}

	void get_start(server_info& _server, echo_session& http_obj)
	{
		do_resolve(_server, http_obj);
	}

private:

	void network_entity(char *buffer)
	{
		char temp[4096] = {0};
		int k = 0, len = 0;
		char c;
		while((c = buffer[k]))
		{
			switch (c)
			{
				case '<':
					strncat(temp, "&lt;", 4);
					len += 4;
					break;
				case '>':
					strncat(temp, "&gt;", 4);
					len += 4;
					break;
				case ' ':
					strncat(temp,"&nbsp;",6);
					len += 6;
					break;
				case '\"':
					strncat(temp, "&quot;",strlen("&quot;"));
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


	void do_resolve(server_info& _server, echo_session& http_obj)
	{
		tcp::resolver::query query(_server.domain_name, _server.port);
		_resolver.async_resolve(query, [&](const boost::system::error_code ec, tcp::resolver::iterator it)
		{
			if (!ec)
			{
				do_connect(_server, it, http_obj);
			}
			else
			{
				cout << "async resolve: " << ec << endl;

			}
		});
	}

	void do_connect(server_info& _server, tcp::resolver::iterator& it, echo_session& http_obj)
	{
		tcp::endpoint _endpoint = it->endpoint();
		_socket.async_connect(_endpoint, [&](boost::system::error_code ec)
		{
			if (!ec)
			{
				// connect success
				my_shell(_server, http_obj);
			}
			else
			{
				cout << "async connect:" << ec << endl;
			}
		});

		return;
	}

	void my_shell(server_info& _server, echo_session& http_obj)
	{
		// cout << "Start doing my_shell... " << endl;
		do_read(_server, http_obj);
	}

	void do_read(server_info& _server, echo_session& http_obj)
	{
		_socket.async_read_some(
			boost::asio::buffer(message, max_length),
			[&](const boost::system::error_code ec, size_t length)
		{
			if (!ec)
			{
				output_shell(message, _server, http_obj);
				if (contain_prompt(message))
				{
					memset(message, 0, sizeof(message));
					send_command(_server, http_obj);
				}		
				memset(message, 0, sizeof(message));
				do_read(_server, http_obj);

			}
			else
			{
				cout << "Async read: " << ec << endl;
			}
		});

		return;
	}

	bool contain_prompt(char *_message)
	{
		for (int i = 0; i < strlen(_message); ++i)
		{
			if (_message[i] == '%' && _message[i + 1] == ' ')
			{
				return true;
			}
		}
		return false;
	}

	void output_shell(char *_message, server_info& _server, echo_session& http_obj)
	{
		string content = string(_message);
		stringstream ss(content);
		string item;
		char *_temp;
		char temp[2048] = { 0 };
		char buffer[2048] = { 0 };

		while (getline(ss, item, '\n'))
		{
			memset(temp, 0, sizeof(temp));
			if (item != "% ")
				item.append("&NewLine;");

			_temp = _strdup(item.c_str());

			strncpy_s(buffer, _temp, strlen(_temp));
			network_entity(buffer);
			sprintf_s(temp, "<script>document.getElementById(\'s%d\').innerHTML += \'%s\';</script>\n", _server.server_index, buffer);

			http_obj._socket.async_send(
				boost::asio::buffer(temp, strlen(temp)), [this](const boost::system::error_code ec, size_t write_len)
			{
				if (!ec)
				{
					// success.
				}
				else
				{
					cout << "output_shell async: " << ec << endl;
				}
			});
		}
	}

	void send_command(server_info& _server, echo_session& http_obj)
	{
		char buffer[4096] = { 0 };
		char temp[4096] = { 0 };
		char *_temp;
		char str[4096] = { 0 };
		string content;
		size_t n;

		if (!fin.getline(buffer, 4096))
		{
			fin.close();
			return;
		}
		n = strlen(buffer);
		content = string(buffer);

		while (content.back() == '\r' || content.back() == '\n')
		{
			content.pop_back();
		}
		content.append("&NewLine;");
		_temp = _strdup(content.c_str());
		strncpy_s(str, _temp, strlen(_temp));
		network_entity(str);

		sprintf_s(temp, "<script>document.getElementById(\'s%d\').innerHTML += \'<b>%s</b>\'; </script>\n", _server.server_index, str);

		http_obj._socket.async_send(
			boost::asio::buffer(temp, strlen(temp)), [this](const boost::system::error_code ec, size_t write_length)
		{
			if (!ec)
			{
				// success
			}
			else
			{
				cout << "send_command async: " << ec << endl;
			}
		});

		buffer[n] = '\n';
		do_write(buffer, n + 1);
	}

	/*
		write data to server.
	*/
	void do_write(char *buffer, size_t length)
	{		
		_socket.async_write_some(
			boost::asio::buffer(buffer, length), [&](const boost::system::error_code ec, size_t write_length) {});
		return;
	}

public:

	ifstream fin;

private:

	tcp::resolver _resolver;
	tcp::socket _socket;
	enum { max_length = 4096 };
	char message[max_length] = { 0 };

};
	
};




class tcp_server
{
public:
	tcp_server(unsigned short port)
		: _socket(global_io_service),
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
			}
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
	_putenv("PATH=.;./test_case");
	// setenv for path.	
	try
	{
		tcp_server http_server(port);
		global_io_service.run();
	}
	catch (std::exception &e)
	{
		cerr << "Exception: " << e.what() << endl;
	}
	return 0;
}
