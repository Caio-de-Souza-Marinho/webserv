#include "../include/RequestParser.hpp"
#include "../include/Request.hpp"
#include "TestUtils.hpp"
#include "TestUtils.hpp"

#include <sstream>

void	testRequestParserSimpleGet()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("GET"), request.method);
	ASSERT_EQ(std::string("/index.html"), request.rawUri);
	ASSERT_EQ(std::string("HTTP/1.1"), request.version);
	ASSERT_EQ(std::string("localhost"), request.host);
}

void	testRequestParserPost()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello world";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("POST"), request.method);
	ASSERT_EQ(std::string("/upload"), request.rawUri);
	ASSERT_EQ(std::string("hello world"), request.body);
}

void	testRequestParserDelete()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"DELETE /file.txt HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("DELETE"), request.method);
	ASSERT_EQ(std::string("/file.txt"), request.rawUri);
}

void	testRequestParserMissingHost()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"GET /index.html HTTP/1.1\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::PARSE_ERROR, state);
}

void	testRequestParserInvalidMethod()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"BREW /coffee HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::PARSE_ERROR, state);
}

void	testRequestParserBodyTooLarge()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer;

	std::string	body(2000, 'A');

	std::ostringstream	oss;

	oss << "POST /upload HTTP/1.1\r\n";
	oss << "Host: localhost\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "\r\n";
	oss << body;

	buffer = oss.str();

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000);

	ASSERT_EQ(RequestParser::PARSE_ERROR, state);
	ASSERT_EQ(413, request.errorCode);
}

void	testRequestParserPartialRequest()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"GET /index.html HTTP/1.1\r\n"
		"Host: local";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::HEADERS, state);
}

void	testRequestParserChunked()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"Hello\r\n"
		"6\r\n"
		" World\r\n"
		"0\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("POST"), request.method);
	ASSERT_EQ(std::string("/upload"), request.rawUri);
	ASSERT_EQ(std::string("Hello World"), request.body);
}

void	testRequestParserIncrementalRead()
{
	RequestParser	parser;
	Request		request;

	std::string	part1 =
		"GET /index.html HTTP/1.1\r\n"
		"Host: loca";

	RequestParser::State	state;

	state = parser.parse(request, part1, 1000000);

	ASSERT_EQ(RequestParser::HEADERS, state);

	part1 +=
		"lhost\r\n"
		"\r\n";

	state = parser.parse(request, part1, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("localhost"), request.host);
}

void	testRequestParser()
{
	testRequestParserSimpleGet();
	testRequestParserPost();
	testRequestParserDelete();
	testRequestParserMissingHost();
	testRequestParserInvalidMethod();
	testRequestParserBodyTooLarge();
	testRequestParserPartialRequest();
	testRequestParserChunked();
	testRequestParserIncrementalRead();
}
