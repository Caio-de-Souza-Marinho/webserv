#include "../include/Router.hpp"
#include "../include/Config.hpp"
#include "../include/Request.hpp"
#include "TestUtils.hpp"

// ------------------------------------------------------- helpers -------------------------------------------------
static ServerConfig	makeConfig()
{
	ServerConfig	config;

	config.host = "localhost";
	config.port = 8080;
	config.maxBodySize = 1000000;

	Route	root;
	root.path = "/";
	root.root = "./www";
	root.methods.insert("GET");
	root.methods.insert("POST");
	root.methods.insert("DELETE");
	config.routes.push_back(root);

	Route	uploads;
	uploads.path = "/uploads";
	uploads.root = "./www/uploads";
	uploads.methods.insert("GET");
	uploads.methods.insert("POST");
	uploads.uploadPath = "./www/uploads";
	config.routes.push_back(uploads);

	Route	readonly;
	readonly.path = "/readonly";
	readonly.root = "./www/readonly";
	readonly.methods.insert("GET");
	config.routes.push_back(readonly);

	Route	redir;
	redir.path = "/redirect";
	redir.redirectCode = 302;
	redir.redirectUrl = "/";
	redir.methods.insert("GET");
	config.routes.push_back(redir);

	Route	cgi;
	cgi.path = "/cgi-bin";
	cgi.root = "./www/cgi-bin";
	cgi.methods.insert("GET");
	cgi.methods.insert("POST");
	cgi.cgiHandlers[".py"] = "/usr/bin/python3";
	cgi.cgiHandlers[".php"] = "/usr/bin/php-cgi";
	config.routes.push_back(cgi);

	return (config);
}

static Request	makeRequest(const std::string &path, const std::string &method = "GET")
{
	Request	req;
	req.path = path;
	req.method = method;
	return (req);
}

// ------------------------------------------------------- mathRoute -------------------------------------------------
void	testRouterMatchRoot()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/index.html");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_EQ(std::string("/"), route->path);
}

void	testRouterMatchExact()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/uploads");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_EQ(std::string("/uploads"), route->path);
}

void	testRouterMatchPrefix()
{
	Router		router;
	ServerConfig	config = makeConfig();

	// /uploads/file.txt should match /uploads, not /
	Request		req = makeRequest("/uploads/file.txt");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_EQ(std::string("/uploads"), route->path);
}

void	testRouterLongestPrefixWins()
{
	Router		router;
	ServerConfig	config = makeConfig();

	// /cgi-bin/test.py: both "/" and "/cgi-bin" match, "/cgi-bin" is longer
	Request		req = makeRequest("/cgi-bin/test.py");
	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_EQ(std::string("/cgi-bin"), route->path);
}

void	testRouterNoRoutes()
{
	Router		router;
	ServerConfig	config;		// empty config, no routes
	Request		req = makeRequest("/anything");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route == NULL);
}

void	testRouterRootCatchesEverything()
{
	Router		router;
	ServerConfig	config = makeConfig();

	// A path that doesn't match any specific route still hits /
	Request		req = makeRequest("/some/deep/path/that/does/not/exist");
	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_EQ(std::string("/"), route->path);
}

// ------------------------------------------------------- isMethodAllowed -------------------------------------------------
void	testRouterMethodAllowed()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/readonly", "GET");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_TRUE(router.isMethodAllowed(*route, "GET"));
}

void	testRouterMethodNotAllowed()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/readonly", "POST");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_TRUE(!router.isMethodAllowed(*route, "POST"));
}

void	testRouterMethodDeleteNotAllowed()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/uploads", "DELETE");

	const Route	*route = router.matchRoute(req, config);

	ASSERT_TRUE(route != NULL);
	ASSERT_TRUE(!router.isMethodAllowed(*route, "DELETE"));
}

// ------------------------------------------------------- resolvePath -------------------------------------------------
void	testRouterResolveRootRoute()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/index.html");

	const Route	*route = router.matchRoute(req, config);
	std::string	path = router.resolvePath(*route, req);

	// route.root = "./www", request.path = "/index.html"
	ASSERT_EQ(std::string("./www/index.html"), path);
}

void	testRouterResolveSubRoute()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/uploads/file.txt");

	const Route	*route = router.matchRoute(req, config);
	std::string	path = router.resolvePath(*route, req);

	// route.root = "./www/uploads", strips "/uploads" prefix → "/file.txt"
	ASSERT_EQ(std::string("./www/uploads/file.txt"), path);
}

void	testRouterResolveExactMatch()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/uploads");

	const Route	*route = router.matchRoute(req, config);
	std::string	path = router.resolvePath(*route, req);

	// path suffix after stripping "/uploads" is "" → root only
	ASSERT_EQ(std::string("./www/uploads"), path);
}

// ------------------------------------------------------- mathCGI -------------------------------------------------
void	testRouterMatchCgiPython()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/cgi-bin/test.py");

	const Route		*route = router.matchRoute(req, config);
	const std::string	*interp = router.matchCGI(*route, req.path);

	ASSERT_TRUE(interp != NULL);
	ASSERT_EQ(std::string("/usr/bin/python3"), *interp);
}

void	testRouterMatchCgiPhp()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/cgi-bin/script.php");

	const Route		*route = router.matchRoute(req, config);
	const std::string	*interp = router.matchCGI(*route, req.path);

	ASSERT_TRUE(interp != NULL);
	ASSERT_EQ(std::string("/usr/bin/php-cgi"), *interp);
}

void	testRouterMatchCgiNoMatch()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/cgi-bin/file.txt");

	const Route		*route = router.matchRoute(req, config);
	const std::string	*interp = router.matchCGI(*route, req.path);

	ASSERT_TRUE(interp == NULL);
}

void	testRouterMatchCgiOnNonCgiRoute()
{
	Router		router;
	ServerConfig	config = makeConfig();
	Request		req = makeRequest("/index.py");	// root route has no CGI handlers

	const Route		*route = router.matchRoute(req, config);
	const std::string	*interp = router.matchCGI(*route, req.path);

	ASSERT_TRUE(interp == NULL);
}

// ------------------------------------------------------- entry point -------------------------------------------------
void	testRouter()
{
	RUN_TEST(testRouterMatchRoot);
	RUN_TEST(testRouterMatchExact);
	RUN_TEST(testRouterMatchPrefix);
	RUN_TEST(testRouterLongestPrefixWins);
	RUN_TEST(testRouterNoRoutes);
	RUN_TEST(testRouterRootCatchesEverything);
	RUN_TEST(testRouterMethodAllowed);
	RUN_TEST(testRouterMethodNotAllowed);
	RUN_TEST(testRouterMethodDeleteNotAllowed);
	RUN_TEST(testRouterResolveRootRoute);
	RUN_TEST(testRouterResolveSubRoute);
	RUN_TEST(testRouterResolveExactMatch);
	RUN_TEST(testRouterMatchCgiPython);
	RUN_TEST(testRouterMatchCgiPhp);
	RUN_TEST(testRouterMatchCgiNoMatch);
	RUN_TEST(testRouterMatchCgiOnNonCgiRoute);
}
