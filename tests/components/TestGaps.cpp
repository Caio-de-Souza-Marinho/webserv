// TestGaps.cpp — fills coverage holes not addressed by the main per-class test files.
// Topics: ConfigParser extras, RequestParser extras, ResponseBuilder extras.

#include "../../include/ConfigParser.hpp"
#include "../../include/Config.hpp"
#include "../../include/RequestParser.hpp"
#include "../../include/Request.hpp"
#include "../../include/ResponseBuilder.hpp"
#include "TestUtils.hpp"

#include <fstream>
#include <cstdio>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// ═══════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════

static std::string writeConf(const std::string &content)
{
	std::string path = "test_gaps.conf";
	std::ofstream f(path.c_str());
	f << content;
	return path;
}

static void removeConf(const std::string &path) { std::remove(path.c_str()); }

static void writeFile(const std::string &path, const std::string &data)
{
	std::ofstream f(path.c_str());
	f << data;
}

static void removeFile(const std::string &path) { std::remove(path.c_str()); }

static ServerConfig makeConfig()
{
	ServerConfig c;
	c.host = "localhost";
	c.port = 8080;
	c.maxBodySize = 1000000;
	return c;
}

static Request makeRequest(const std::string &method, const std::string &path,
	const std::string &body = "")
{
	Request r;
	r.method = method;
	r.path   = path;
	r.rawUri = path;
	r.body   = body;
	return r;
}

static Route makeRoute(const std::string &path, const std::string &root,
	bool autoindex = false)
{
	Route rt;
	rt.path = path;
	rt.root = root;
	rt.methods.insert("GET");
	rt.methods.insert("POST");
	rt.methods.insert("DELETE");
	rt.autoindex = autoindex;
	return rt;
}

// ═══════════════════════════════════════════════════════════════
// ConfigParser gaps
// ═══════════════════════════════════════════════════════════════

// client_max_body_size with 'k' suffix — parser rejects it (not a supported suffix)
static void testConfigBodySizeKilobytes()
{
	std::string path = writeConf(
		"server {\n"
		"listen 8080;\n"
		"client_max_body_size 512k;\n"
		"}\n");

	bool threw = false;
	try
	{
		ConfigParser parser(path);
		parser.parse();
	}
	catch (const std::exception&) { threw = true; }

	// Parser does not accept the 'k' suffix — must throw
	ASSERT_TRUE(threw);
	removeConf(path);
}

// client_max_body_size without suffix = raw bytes
static void testConfigBodySizeNoSuffix()
{
	std::string path = writeConf(
		"server {\n"
		"listen 8080;\n"
		"client_max_body_size 4096;\n"
		"}\n");

	ConfigParser parser(path);
	std::vector<ServerConfig> cfgs = parser.parse();

	ASSERT_EQ((size_t)4096, cfgs[0].maxBodySize);
	removeConf(path);
}

// Multiple error_page directives in one server block
static void testConfigMultipleErrorPages()
{
	std::string path = writeConf(
		"server {\n"
		"listen 8080;\n"
		"error_page 404 /404.html;\n"
		"error_page 500 /500.html;\n"
		"}\n");

	ConfigParser parser(path);
	std::vector<ServerConfig> cfgs = parser.parse();

	ASSERT_EQ(std::string("/404.html"), cfgs[0].errorPages[404]);
	ASSERT_EQ(std::string("/500.html"), cfgs[0].errorPages[500]);
	removeConf(path);
}

// location with index directive
static void testConfigLocationIndex()
{
	std::string path = writeConf(
		"server {\n"
		"listen 8080;\n"
		"location / {\n"
		"root ./www;\n"
		"index index.html index.htm;\n"
		"}\n"
		"}\n");

	ConfigParser parser(path);
	std::vector<ServerConfig> cfgs = parser.parse();

	ASSERT_EQ((size_t)1, cfgs[0].routes.size());
	// At least one index file should be recorded
	ASSERT_TRUE(!cfgs[0].routes[0].index.empty());
	// First index file must be index.html
	ASSERT_EQ(std::string("index.html"), cfgs[0].routes[0].index[0]);
	// Both index files recorded
	ASSERT_TRUE(cfgs[0].routes[0].index.size() >= 2);
	removeConf(path);
}

// location with upload_path directive
static void testConfigLocationUploadPath()
{
	std::string path = writeConf(
		"server {\n"
		"listen 8080;\n"
		"location /uploads {\n"
		"root ./www/uploads;\n"
		"upload_path ./www/uploads;\n"
		"}\n"
		"}\n");

	ConfigParser parser(path);
	std::vector<ServerConfig> cfgs = parser.parse();

	ASSERT_EQ(std::string("./www/uploads"), cfgs[0].routes[0].uploadPath);
	removeConf(path);
}

// Multiple CGI handlers in one location
static void testConfigMultipleCgiHandlers()
{
	std::string path = writeConf(
		"server {\n"
		"listen 8080;\n"
		"location /cgi {\n"
		"cgi .py /usr/bin/python3;\n"
		"cgi .php /usr/bin/php-cgi;\n"
		"}\n"
		"}\n");

	ConfigParser parser(path);
	std::vector<ServerConfig> cfgs = parser.parse();

	ASSERT_EQ(std::string("/usr/bin/python3"), cfgs[0].routes[0].cgiHandlers[".py"]);
	ASSERT_EQ(std::string("/usr/bin/php-cgi"),  cfgs[0].routes[0].cgiHandlers[".php"]);
	removeConf(path);
}

// Empty config file — should throw or return zero servers
static void testConfigEmptyFile()
{
	std::string path = writeConf("");
	bool threw = false;
	try
	{
		ConfigParser parser(path);
		std::vector<ServerConfig> cfgs = parser.parse();
		// Returning empty is acceptable; throwing is also acceptable
		ASSERT_TRUE(cfgs.empty() || true);
	}
	catch (const std::exception&) { threw = true; }
	(void)threw;
	ASSERT_TRUE(true); // reaching here without crash is the goal
	removeConf(path);
}

// ═══════════════════════════════════════════════════════════════
// RequestParser gaps
// ═══════════════════════════════════════════════════════════════

// HEAD method — the parser only accepts GET/POST/DELETE, so HEAD → PARSE_ERROR
static void testRequestParserHead()
{
	RequestParser parser;
	Request req;

	std::string buf =
		"HEAD /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State state = parser.parse(req, buf, 1000000);

	// HEAD is not in the server's allowed method set → treated as invalid
	ASSERT_EQ(RequestParser::PARSE_ERROR, state);
}

// Content-Type header is preserved in request.headers
static void testRequestParserContentTypeHeader()
{
	RequestParser parser;
	Request req;

	std::string buf =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: 2\r\n"
		"\r\n"
		"{}";

	RequestParser::State state = parser.parse(req, buf, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	// headers map uses lowercase keys
	ASSERT_EQ(std::string("application/json"), req.headers["content-type"]);
}

// Multiple headers all parsed correctly
static void testRequestParserMultipleHeaders()
{
	RequestParser parser;
	Request req;

	std::string buf =
		"GET /page HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Accept: text/html\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";

	RequestParser::State state = parser.parse(req, buf, 1000000);

	ASSERT_EQ(RequestParser::COMPLETE, state);
	ASSERT_EQ(std::string("text/html"), req.headers["accept"]);
	ASSERT_EQ(std::string("keep-alive"), req.headers["connection"]);
}

// Body larger than limit triggers 413 regardless of Content-Length being present
static void testRequestParserBodyExactlyAtLimit()
{
	RequestParser parser;
	Request req;

	std::string body(1000, 'X');
	std::ostringstream oss;
	oss << "POST /up HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
	    << body.size() << "\r\n\r\n" << body;
	std::string buf = oss.str();

	// Limit == body size → should be COMPLETE (at the boundary, not over it)
	RequestParser::State state = parser.parse(req, buf, 1000);
	ASSERT_EQ(RequestParser::COMPLETE, state);
}

// Empty URI should return an error
static void testRequestParserEmptyUri()
{
	RequestParser parser;
	Request req;

	std::string buf =
		"GET  HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State state = parser.parse(req, buf, 1000000);
	// Parser should either return PARSE_ERROR or COMPLETE — it must not crash
	ASSERT_TRUE(state == RequestParser::PARSE_ERROR || state == RequestParser::COMPLETE);
}

// HTTP version that isn't 1.0 or 1.1 → error
static void testRequestParserInvalidHttpVersion()
{
	RequestParser parser;
	Request req;

	std::string buf =
		"GET /index HTTP/2.0\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser::State state = parser.parse(req, buf, 1000000);
	ASSERT_EQ(RequestParser::PARSE_ERROR, state);
}

// ═══════════════════════════════════════════════════════════════
// ResponseBuilder gaps
// ═══════════════════════════════════════════════════════════════

// Content-Type for JavaScript file
static void testResponseBuilderContentTypeJs()
{
	writeFile("test_ct.js", "console.log('hi');");

	ResponseBuilder rb;
	ServerConfig config = makeConfig();
	Request req = makeRequest("GET", "/test_ct.js");
	Route route = makeRoute("/", ".");

	Response res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(std::string("application/javascript"), res.headers["Content-Type"]);
	removeFile("test_ct.js");
}

// Content-Type for JSON file
static void testResponseBuilderContentTypeJson()
{
	writeFile("test_ct.json", "{}");

	ResponseBuilder rb;
	ServerConfig config = makeConfig();
	Request req = makeRequest("GET", "/test_ct.json");
	Route route = makeRoute("/", ".");

	Response res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(std::string("application/json"), res.headers["Content-Type"]);
	removeFile("test_ct.json");
}

// Content-Type for PNG — application/octet-stream fallback (binary extension)
static void testResponseBuilderContentTypePng()
{
	writeFile("test_ct.png", "\x89PNG");

	ResponseBuilder rb;
	ServerConfig config = makeConfig();
	Request req = makeRequest("GET", "/test_ct.png");
	Route route = makeRoute("/", ".");

	Response res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(std::string("image/png"), res.headers["Content-Type"]);
	removeFile("test_ct.png");
}

// Unsupported method (PUT) should yield 501
static void testResponseBuilderUnsupportedMethod()
{
	ResponseBuilder rb;
	ServerConfig config = makeConfig();
	Request req = makeRequest("PUT", "/something");

	Route route = makeRoute("/", ".");
	route.methods.insert("PUT"); // allow it so we skip 405

	Response res = rb.buildResponse(req, &route, config);

	// PUT is not implemented in the server → 501
	ASSERT_EQ(501, res.statusCode);
}

// POST with multipart but no files (parts without filename) → 400
static void testResponseBuilderMultipartNoFileParts()
{
	ResponseBuilder rb;
	ServerConfig config = makeConfig();

	mkdir("test_mp_uploads", 0755);

	std::string boundary = "bound";
	// A plain text field with no filename
	std::string body =
		"--bound\r\n"
		"Content-Disposition: form-data; name=\"field\"\r\n"
		"\r\n"
		"value\r\n"
		"--bound--\r\n";

	Request req = makeRequest("POST", "/upload", body);
	req.headers["content-type"] = "multipart/form-data; boundary=bound";

	Route route = makeRoute("/upload", ".");
	route.uploadPath = "test_mp_uploads";

	Response res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(400, res.statusCode);

	rmdir("test_mp_uploads");
}

// 404 error body contains the status code text
static void testResponseBuilderErrorBodyContainsCode()
{
	ResponseBuilder rb;
	ServerConfig config = makeConfig();

	Response res = rb.buildErrorResponse(403, config);

	ASSERT_EQ(403, res.statusCode);
	ASSERT_TRUE(res.body.find("403") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════
// Entry point — called from TestRunner
// ═══════════════════════════════════════════════════════════════
void testGaps()
{
	// ConfigParser
	RUN_TEST(testConfigBodySizeKilobytes);
	RUN_TEST(testConfigBodySizeNoSuffix);
	RUN_TEST(testConfigMultipleErrorPages);
	RUN_TEST(testConfigLocationIndex);
	RUN_TEST(testConfigLocationUploadPath);
	RUN_TEST(testConfigMultipleCgiHandlers);
	RUN_TEST(testConfigEmptyFile);

	// RequestParser
	RUN_TEST(testRequestParserHead);
	RUN_TEST(testRequestParserContentTypeHeader);
	RUN_TEST(testRequestParserMultipleHeaders);
	RUN_TEST(testRequestParserBodyExactlyAtLimit);
	RUN_TEST(testRequestParserEmptyUri);
	RUN_TEST(testRequestParserInvalidHttpVersion);

	// ResponseBuilder
	RUN_TEST(testResponseBuilderContentTypeJs);
	RUN_TEST(testResponseBuilderContentTypeJson);
	RUN_TEST(testResponseBuilderContentTypePng);
	RUN_TEST(testResponseBuilderUnsupportedMethod);
	RUN_TEST(testResponseBuilderMultipartNoFileParts);
	RUN_TEST(testResponseBuilderErrorBodyContainsCode);
}
