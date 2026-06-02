/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tester.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 17:45:44 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 17:49:45 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServer.hpp"
#include "../include/Request.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Config.hpp"
#include "../include/Response.hpp"
#include "../include/Router.hpp"
#include "../include/Colors.hpp"

#include <iostream>

// -- Request Tester
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
		parser.parse(request, raw, 1000);
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
		parser.parse(request, raw, 100);
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
		parser.parse(request, raw, 1000);
		printRequest(request);	// body should be hello world
	}

	// test 4 - incomplete request (arrives in two chunks)
	{
		RequestParser	parser;
		Request		request;
		std::string	buffer = "GET /page HTTP/1.1\r\nHost: local";
		std::string	part2 = "host\r\n\r\n";
		parser.parse(request, buffer, 1000);
		std::cout << "after part1 complete: " << request.isComplete << "\n";
		buffer += "host\r\n\r\n";
		parser.parse(request, buffer, 1000);
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
		parser.parse(request, raw, 1000);
		std::cout << "path " << request.path << "\n---\n"; // should be /my file.html
	}
}
// --
//
// -- Tester Response Build
void	printBuiltResponse(const std::string &testName, const Response &res)
{
	std::cout << BLUE << "\n========== " << testName << " ==========\n" << RESET;
	std::cout << res.build() << "\n";
}

void	testResponseBuild(void)
{
	// Test 1 - 200 OK com text/plain
	{
		Response res;

		res.statusCode = 200;
		res.headers["Content-Type"] = "text/plain";
		res.body = "Hello";

		printBuiltResponse("200 OK text/plain", res);
	}

	// Test 2 - 404 Not Found com HTML
	{
		Response res;

		res.statusCode = 404;
		res.headers["Content-Type"] = "text/html";
		res.body = "<html><body><h1>404 Not Found</h1></body></html>";

		printBuiltResponse("404 Not Found HTML", res);
	}

	// Test 3 - 301 Redirect sem body
	{
		Response res;

		res.statusCode = 301;
		res.headers["Location"] = "/new-page";
		res.body = "";

		printBuiltResponse("301 Redirect", res);
	}

	// Test 4 - 204 No Content sem body
	{
		Response res;

		res.statusCode = 204;
		res.body = "";

		printBuiltResponse("204 No Content", res);
	}

	// Test 5 - 500 Internal Server Error sem Content-Type manual
	{
		Response res;

		res.statusCode = 500;
		res.body = "<h1>Internal Server Error</h1>";

		printBuiltResponse("500 default Content-Type", res);
	}
}
// --
//
// -- Tester Resolve Path
void	testResolvePath(const std::string &path)
{
	ServerConfig	config = makeMockConfig();
	Request		req;
	Router		router;
	const Route	*route;

	req.path = path;
	route = router.matchRoute(req, config);

	std::cout << "request: " << path << "\n";
	if (route)
	{
		std::cout << "route:   " << route->path << "\n";
		std::cout << "root:    " << route->root << "\n";
		std::cout << "file:    " << router.resolvePath(*route, req) << "\n";
	}
	else
		std::cout << "route:   NULL\n";
	std::cout << "---\n";
}

