#include "../include/RequestParser.hpp"
#include "../include/Request.hpp"
#include "TestUtils.hpp"
#include "TestUtils.hpp"

void testRequestParserSimpleGet()
{
	RequestParser	parser;
	Request			request;

	std::string buffer =
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State state;

	state = parser.parse(request, buffer, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("GET"), request.method);
	ASSERT_EQ(std::string("/index.html"), request.rawUri);
	ASSERT_EQ(std::string("HTTP/1.1"), request.version);
	ASSERT_EQ(std::string("localhost"), request.host);
}

void	testRequestParser()
{
	testRequestParserSimpleGet();
}
