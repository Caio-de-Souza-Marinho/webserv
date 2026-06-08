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

// HTTP/1.0 should complete without Host (Host not required in 1.0)
void	testRequestParserHttp10NoHost()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"GET /index.html HTTP/1.0\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
}

// URI with query string should split correctly
void	testRequestParserQueryString()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"GET /search?q=hello&page=2 HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("/search"), request.path);
	ASSERT_EQ(std::string("q=hello&page=2"), request.query);
}

// Parser reset should allow reuse for a second request
void	testRequestParserReset()
{
	RequestParser	parser;
	Request		request;

	std::string	buffer =
		"GET /first HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";
	parser.parse(request, buffer, 1000000);

	parser.reset();
	request.reset();

	buffer =
		"GET /second HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State	state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("/second"), request.rawUri);
}

void	testRequestParser()
{
	RUN_TEST(testRequestParserSimpleGet);
	RUN_TEST(testRequestParserPost);
	RUN_TEST(testRequestParserDelete);
	RUN_TEST(testRequestParserMissingHost);
	RUN_TEST(testRequestParserInvalidMethod);
	RUN_TEST(testRequestParserBodyTooLarge);
	RUN_TEST(testRequestParserPartialRequest);
	RUN_TEST(testRequestParserChunked);
	RUN_TEST(testRequestParserIncrementalRead);
	RUN_TEST(testRequestParserHttp10NoHost);
	RUN_TEST(testRequestParserQueryString);
	RUN_TEST(testRequestParserReset);
}
