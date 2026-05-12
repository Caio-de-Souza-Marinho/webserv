#include "../include/WebServer.hpp"
#include "../include/Request.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Config.hpp"
#include "../include/Response.hpp"
#include "../include/Colors.hpp"

#include <iostream>
/*
int	main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	std::cout << "hello world\n";
	ServerConfig Config = makeMockConfig();
	return (0);
}
*/

void	printRequest(const Request &request)
{
	std::cout << "host:		" << request.host << "\n";
	std::cout << "method:		" << request.method << "\n";
	std::cout << "path:		" << request.path << "\n";
	std::cout << "query:		" << request.query << "\n";
	std::cout << "version:		" << request.version << "\n";
	std::cout << "keepAlive:	" << request.keepAlive << "\n";
	std::cout << "chunked:		" << request.isChunked << "\n";
	std::cout << "complete:		" << request.isComplete << "\n";
	std::cout << "body:		" << request.body << "\n";
	std::cout << "---\n";
}

void	testRequestParser(void)
{

	// test 1 - simple GET
	{
		RequestParser	parser;
		Request		request;
		std::string	raw = 
			"GET /index.html?foo=bar HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"Connection: keep-alive\r\n"
			"\r\n";
		parser.parse(request, raw);
		printRequest(request);
	}

	// test 2 - POST with body
	{
		RequestParser	parser;
		Request		request;
		std::string	raw = 
			"POST /upload HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"Content-Length: 11\r\n"
			"\r\n"
			"hello world";
		parser.parse(request, raw);
		printRequest(request);
	}

	// test 3 - chunked body
	{
		RequestParser	parser;
		Request		request;
		std::string	raw = 
			"POST /data HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\nhello\r\n"
			"6\r\n world\r\n"
			"0\r\n\r\n";
		parser.parse(request, raw);
		printRequest(request);	// body should be hello world
	}

	// test 4 - incomplete request (arrives in two chunks)
	{
		RequestParser	parser;
		Request		request;
		std::string	buffer = "GET /page HTTP/1.1\r\nHost: local";
		std::string	part2 = "host\r\n\r\n";
		parser.parse(request, buffer);
		std::cout << "after part1 complete: " << request.isComplete << "\n";
		buffer += "host\r\n\r\n";
		parser.parse(request, buffer);
		std::cout << "after part2 complete: " << request.isComplete << "\n---\n";
	}

	// test 5 - percent encoded path
	{
		RequestParser	parser;
		Request		request;
		std::string	raw = 
			"GET /my%20file.html HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";
		parser.parse(request, raw);
		std::cout << "path " << request.path << "\n---\n"; // should be /my file.html
	}
}

void	testResponseBuild()
{
	Response res;

	res.statusCode = 404;
	res.headers["Content-Type"] = "text/html";
	res.body = "<h1>Not Found</h1>";

	std::cout << res.build() << std::endl;
}

int	main()
{
	// make ServerConfig (Mock)
	ServerConfig Config = makeMockConfig();

	std::cout << "----------------------------" << std::endl;
	std::cout << GREEN << "Test Resquest Parser: " << RESET << std::endl;
	testRequestParser();
	std::cout << "----------------------------" << std::endl;
	std::cout << GREEN << "Test Response Build: " << RESET << std::endl;
	testResponseBuild();
	std::cout << "----------------------------" << std::endl;
	return (0);
}

