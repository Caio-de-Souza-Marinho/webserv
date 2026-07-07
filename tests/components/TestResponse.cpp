#include "../../include/Response.hpp"
#include "TestUtils.hpp"

#include <string>
#include <cstddef>

// ------------------------------------------------------- getStatusMessage -------------------------------------------------

static void testStatusMessage200()
{
	ASSERT_EQ(std::string("OK"), Response::getStatusMessage(200));
}

static void testStatusMessage201()
{
	ASSERT_EQ(std::string("Created"), Response::getStatusMessage(201));
}

static void testStatusMessage204()
{
	ASSERT_EQ(std::string("No Content"), Response::getStatusMessage(204));
}

static void testStatusMessage301()
{
	ASSERT_EQ(std::string("Moved Permanently"), Response::getStatusMessage(301));
}

static void testStatusMessage302()
{
	ASSERT_EQ(std::string("Found"), Response::getStatusMessage(302));
}

static void testStatusMessage400()
{
	ASSERT_EQ(std::string("Bad Request"), Response::getStatusMessage(400));
}

static void testStatusMessage403()
{
	ASSERT_EQ(std::string("Forbidden"), Response::getStatusMessage(403));
}

static void testStatusMessage404()
{
	ASSERT_EQ(std::string("Not Found"), Response::getStatusMessage(404));
}

static void testStatusMessage405()
{
	ASSERT_EQ(std::string("Method Not Allowed"), Response::getStatusMessage(405));
}

static void testStatusMessage408()
{
	ASSERT_EQ(std::string("Request Timeout"), Response::getStatusMessage(408));
}

static void testStatusMessage413()
{
	ASSERT_EQ(std::string("Content Too Large"), Response::getStatusMessage(413));
}

static void testStatusMessage500()
{
	ASSERT_EQ(std::string("Internal Server Error"), Response::getStatusMessage(500));
}

static void testStatusMessage501()
{
	ASSERT_EQ(std::string("Not Implemented"), Response::getStatusMessage(501));
}

static void testStatusMessage502()
{
	ASSERT_EQ(std::string("Bad Gateway"), Response::getStatusMessage(502));
}

static void testStatusMessageUnknown()
{
	// Any unmapped code returns "Unknown"
	ASSERT_EQ(std::string("Unknown"), Response::getStatusMessage(999));
	ASSERT_EQ(std::string("Unknown"), Response::getStatusMessage(0));
}

// ------------------------------------------------------- build() structure -------------------------------------------------

static void testBuildStatusLine()
{
	Response r;
	r.statusCode = 200;
	r.body = "hello";

	std::string raw = r.build();

	// Must start with the HTTP/1.1 status line
	ASSERT_TRUE(raw.find("HTTP/1.1 200 OK") == 0);
}

static void testBuildContainsBody()
{
	Response r;
	r.statusCode = 200;
	r.body = "hello world";

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("hello world") != std::string::npos);
}

static void testBuildContentLengthMatchesBody()
{
	Response r;
	r.statusCode = 200;
	r.body = "abcde";

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Content-Length: 5") != std::string::npos);
}

static void testBuildContentLengthZeroForEmptyBody()
{
	Response r;
	r.statusCode = 204;
	// body intentionally empty

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Content-Length: 0") != std::string::npos);
}

static void testBuildContainsServerHeader()
{
	Response r;
	r.statusCode = 200;
	r.body = "x";

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Server: webserv/1.0") != std::string::npos);
}

static void testBuildContainsDateHeader()
{
	Response r;
	r.statusCode = 200;
	r.body = "x";

	std::string raw = r.build();

	// Date header must be present (exact value varies)
	ASSERT_TRUE(raw.find("Date: ") != std::string::npos);
}

static void testBuildConnectionCloseByDefault()
{
	Response r;
	r.statusCode = 200;
	r.body = "x";
	// keepAlive defaults to false

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Connection: close") != std::string::npos);
}

static void testBuildConnectionKeepAlive()
{
	Response r;
	r.statusCode = 200;
	r.body = "x";
	r.keepAlive = true;

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Connection: keep-alive") != std::string::npos);
}

static void testBuildCustomHeaderPreserved()
{
	Response r;
	r.statusCode = 301;
	r.headers["Location"] = "/new-path";

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Location: /new-path") != std::string::npos);
}

static void testBuildDefaultContentTypeForBodyNoHeader()
{
	Response r;
	r.statusCode = 200;
	r.body = "<h1>hi</h1>";
	// No Content-Type set explicitly

	std::string raw = r.build();

	// build() injects text/html when body is non-empty and no Content-Type set
	ASSERT_TRUE(raw.find("Content-Type: text/html") != std::string::npos);
}

static void testBuildExplicitContentTypeRespected()
{
	Response r;
	r.statusCode = 200;
	r.headers["Content-Type"] = "application/json";
	r.body = "{}";

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Content-Type: application/json") != std::string::npos);
}

static void testBuildHeaderBodySeparatedByCRLF()
{
	Response r;
	r.statusCode = 200;
	r.body = "payload";

	std::string raw = r.build();

	// Headers and body must be separated by \r\n\r\n
	ASSERT_TRUE(raw.find("\r\n\r\n") != std::string::npos);

	// Body must appear after the blank line
	size_t sep = raw.find("\r\n\r\n");
	ASSERT_TRUE(raw.substr(sep + 4).find("payload") == 0);
}

static void testBuildLargeBody()
{
	Response r;
	r.statusCode = 200;
	r.body = std::string(65536, 'Z');

	std::string raw = r.build();

	ASSERT_TRUE(raw.find("Content-Length: 65536") != std::string::npos);
	ASSERT_TRUE(raw.find(std::string(65536, 'Z')) != std::string::npos);
}

// ------------------------------------------------------- entry point -------------------------------------------------
void testResponse()
{
	RUN_TEST(testStatusMessage200);
	RUN_TEST(testStatusMessage201);
	RUN_TEST(testStatusMessage204);
	RUN_TEST(testStatusMessage301);
	RUN_TEST(testStatusMessage302);
	RUN_TEST(testStatusMessage400);
	RUN_TEST(testStatusMessage403);
	RUN_TEST(testStatusMessage404);
	RUN_TEST(testStatusMessage405);
	RUN_TEST(testStatusMessage408);
	RUN_TEST(testStatusMessage413);
	RUN_TEST(testStatusMessage500);
	RUN_TEST(testStatusMessage501);
	RUN_TEST(testStatusMessage502);
	RUN_TEST(testStatusMessageUnknown);
	RUN_TEST(testBuildStatusLine);
	RUN_TEST(testBuildContainsBody);
	RUN_TEST(testBuildContentLengthMatchesBody);
	RUN_TEST(testBuildContentLengthZeroForEmptyBody);
	RUN_TEST(testBuildContainsServerHeader);
	RUN_TEST(testBuildContainsDateHeader);
	RUN_TEST(testBuildConnectionCloseByDefault);
	RUN_TEST(testBuildConnectionKeepAlive);
	RUN_TEST(testBuildCustomHeaderPreserved);
	RUN_TEST(testBuildDefaultContentTypeForBodyNoHeader);
	RUN_TEST(testBuildExplicitContentTypeRespected);
	RUN_TEST(testBuildHeaderBodySeparatedByCRLF);
	RUN_TEST(testBuildLargeBody);
}
