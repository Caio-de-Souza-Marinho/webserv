#include "../include/ResponseBuilder.hpp"
#include "../include/Request.hpp"
#include "../include/Config.hpp"
#include "TestUtils.hpp"

#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

// ------------------------------------------------------- helpers -------------------------------------------------
static void	writeFile(const std::string &path, const std::string &content)
{
	std::ofstream	file(path.c_str());
	file << content;
}

static void	removeFile(const std::string &path)
{
	std::remove(path.c_str());
}

static ServerConfig	makeConfig()
{
	ServerConfig	config;
	config.host = "localhost";
	config.port = 8080;
	config.maxBodySize = 1000000;
	return (config);
}

static Request	makeRequest(const std::string &method, const std::string &path, const std::string &body = "")
{
	Request	req;
	req.method = method;
	req.path = path;
	req.rawUri = path;
	req.body = body;
	return (req);
}

static Route	makeRoute(const std::string &path, const std::string &root, bool autoindex = false)
{
	Route	route;
	route.path = path;
	route.root = root;
	route.methods.insert("GET");
	route.methods.insert("POST");
	route.methods.insert("DELETE");
	route.autoindex = autoindex;
	return (route);
}

// ------------------------------------------------------- NULL route -> 404 -------------------------------------------------
void	testResponseBuilderNullRoute()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/missing");

	Response	res = rb.buildResponse(req, NULL, config);

	ASSERT_EQ(404, res.statusCode);
}

// ------------------------------------------------------- redirect -------------------------------------------------

void	testResponseBuilderRedirect()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/old");

	Route	route = makeRoute("/old", "./www");
	route.redirectCode = 301;
	route.redirectUrl = "/new";

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(301, res.statusCode);
	ASSERT_EQ(std::string("/new"), res.headers["Location"]);
}

void	testResponseBuilderRedirect302()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/temp");

	Route	route = makeRoute("/temp", "./www");
	route.redirectCode = 302;
	route.redirectUrl = "/";

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(302, res.statusCode);
	ASSERT_EQ(std::string("/"), res.headers["Location"]);
}

// ------------------------------------------------------- method not allowed-------------------------------------------------

void	testResponseBuilderMethodNotAllowed()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("DELETE", "/page");

	Route	route = makeRoute("/", "./www");
	route.methods.clear();
	route.methods.insert("GET");	// DELETE not allowed

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(405, res.statusCode);
}

// ------------------------------------------------------- GET existing file -------------------------------------------------

void	testResponseBuilderGetExistingFile()
{
	writeFile("test_response_file.html", "<h1>Hello</h1>");

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/test_response_file.html");

	Route	route = makeRoute("/", ".");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(200, res.statusCode);
	ASSERT_EQ(std::string("<h1>Hello</h1>"), res.body);

	removeFile("test_response_file.html");
}

// ------------------------------------------------------- GET missing file -> 404 -------------------------------------------------

void	testResponseBuilderGetMissingFile()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/does_not_exist.html");

	Route	route = makeRoute("/", "./www");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(404, res.statusCode);
}

// ------------------------------------------------------- GET content-type -------------------------------------------------

void	testResponseBuilderContentTypeHtml()
{
	writeFile("test_ct.html", "<p>hi</p>");

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/test_ct.html");

	Route	route = makeRoute("/", ".");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(std::string("text/html"), res.headers["Content-Type"]);

	removeFile("test_ct.html");
}

void	testResponseBuilderContentTypeCss()
{
	writeFile("test_ct.css", "body { color: red; }");

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/test_ct.css");

	Route	route = makeRoute("/", ".");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(std::string("text/css"), res.headers["Content-Type"]);

	removeFile("test_ct.css");
}

// ------------------------------------------------------- directory: autoindex off -> 403 -------------------------------------------------

void	testResponseBuilderDirectoryAutoindexOff()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/");

	Route	route = makeRoute("/", ".");
	route.autoindex = false;
	// no index files configured

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(403, res.statusCode);
}

// ------------------------------------------------------- directory: autoindex on -> 200 with listing -------------------------------------------------

void	testResponseBuilderDirectoryAutoindexOn()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/");

	Route	route = makeRoute("/", ".");
	route.autoindex = true;

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(200, res.statusCode);
	// body should contain an HTML listing
	ASSERT_TRUE(res.body.find("<html>") != std::string::npos);
}

// ------------------------------------------------------- directory: index file served -------------------------------------------------

void	testResponseBuilderDirectoryServesIndex()
{
	writeFile("test_idx_dir_index.html", "<h1>Index</h1>");

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("GET", "/");

	Route	route = makeRoute("/", ".");
	route.autoindex = false;
	route.index.push_back("test_idx_dir_index.html");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(200, res.statusCode);
	ASSERT_EQ(std::string("<h1>Index</h1>"), res.body);

	removeFile("test_idx_dir_index.html");
}

// ------------------------------------------------------- POST upload -------------------------------------------------

void	testResponseBuilderPostUpload()
{
	// ensure the upload directory exists
	mkdir("test_uploads_dir", 0755);

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("POST", "/upload", "file content here");

	Route	route = makeRoute("/upload", ".");
	route.uploadPath = "test_uploads_dir";

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(201, res.statusCode);
	ASSERT_TRUE(!res.headers["Location"].empty());

	// clean up: remove any file written inside test_uploads_dir
	// (we don't know the generated filename, so just remove the dir contents
	// via the location header the response gave us)
	if (!res.headers["Location"].empty())
		removeFile(res.headers["Location"]);
	rmdir("test_uploads_dir");
}

// ------------------------------------------------------- POST no upload path -> 403 -------------------------------------------------

void	testResponseBuilderPostNoUploadPath()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("POST", "/upload", "data");

	Route	route = makeRoute("/upload", ".");
	// uploadPath intentionally left empty

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(403, res.statusCode);
}

// ------------------------------------------------------- DELETE existing file -------------------------------------------------

void	testResponseBuilderDeleteExistingFile()
{
	writeFile("test_delete_me.txt", "bye");

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("DELETE", "/test_delete_me.txt");

	Route	route = makeRoute("/", ".");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(204, res.statusCode);

	// file should be gone
	std::ifstream	f("test_delete_me.txt");
	ASSERT_TRUE(!f.good());
}

// ------------------------------------------------------- DELETE missing file -> 404 -------------------------------------------------

void	testResponseBuilderDeleteMissingFile()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("DELETE", "/ghost.txt");

	Route	route = makeRoute("/", ".");

	Response	res = rb.buildResponse(req, &route, config);

	ASSERT_EQ(404, res.statusCode);
}

// ------------------------------------------------------- buildErrorResponse -------------------------------------------------

void	testResponseBuilderErrorResponseDefault()
{
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();	// no custom error pages

	Response	res = rb.buildErrorResponse(404, config);

	ASSERT_EQ(404, res.statusCode);
	ASSERT_TRUE(!res.body.empty());
	ASSERT_TRUE(res.body.find("404") != std::string::npos);
}

void	testResponseBuilderErrorResponseCustomPage()
{
	writeFile("test_custom_404.html", "<h1>Custom 404</h1>");

	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	config.errorPages[404] = "test_custom_404.html";

	Response	res = rb.buildErrorResponse(404, config);

	ASSERT_EQ(404, res.statusCode);
	ASSERT_EQ(std::string("<h1>Custom 404</h1>"), res.body);

	removeFile("test_custom_404.html");
}

void	testResponseBuilderErrorResponseMissingCustomPage()
{
	// custom page path configured but file doesn't exist → fallback to default
	ResponseBuilder	rb;
	ServerConfig	config = makeConfig();
	config.errorPages[500] = "nonexistent_error.html";

	Response	res = rb.buildErrorResponse(500, config);

	ASSERT_EQ(500, res.statusCode);
	ASSERT_TRUE(!res.body.empty());	// should have fallen back to default body
}


// ------------------------------------------------------- entry point -------------------------------------------------
void	testResponseBuilder()
{
	RUN_TEST(testResponseBuilderNullRoute);
	RUN_TEST(testResponseBuilderRedirect);
	RUN_TEST(testResponseBuilderRedirect302);
	RUN_TEST(testResponseBuilderMethodNotAllowed);
	RUN_TEST(testResponseBuilderGetExistingFile);
	RUN_TEST(testResponseBuilderGetMissingFile);
	RUN_TEST(testResponseBuilderContentTypeHtml);
	RUN_TEST(testResponseBuilderContentTypeCss);
	RUN_TEST(testResponseBuilderDirectoryAutoindexOff);
	RUN_TEST(testResponseBuilderDirectoryAutoindexOn);
	RUN_TEST(testResponseBuilderDirectoryServesIndex);
	RUN_TEST(testResponseBuilderPostUpload);
	RUN_TEST(testResponseBuilderPostNoUploadPath);
	RUN_TEST(testResponseBuilderDeleteExistingFile);
	RUN_TEST(testResponseBuilderDeleteMissingFile);
	RUN_TEST(testResponseBuilderErrorResponseDefault);
	RUN_TEST(testResponseBuilderErrorResponseCustomPage);
	RUN_TEST(testResponseBuilderErrorResponseMissingCustomPage);
}
