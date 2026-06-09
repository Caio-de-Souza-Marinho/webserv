#include "../include/ConfigParser.hpp"
#include "../include/Config.hpp"
#include "TestUtils.hpp"

#include <fstream>
#include <cstdio>
#include <stdexcept>

// ------------------------------------------------------- helpers -------------------------------------------------
static std::string	createConfigFile(const std::string &content)
{
	std::string	path = "test_config.conf";
	std::ofstream	file(path.c_str());

	file << content;
	file.close();

	return (path);
}

static void	removeConfigFile(const std::string &path)
{
	std::remove(path.c_str());
}

// ------------------------------------------------------- tests -------------------------------------------------
void	testConfigParserSimpleServer()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ((size_t)1, configs.size());
	ASSERT_EQ(8080, configs[0].port);
	ASSERT_EQ(std::string("0.0.0.0"), configs[0].host);

	removeConfigFile(path);
}

void	testConfigParserHostPort()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 127.0.0.1:4242;\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ(std::string("127.0.0.1"), configs[0].host);
	ASSERT_EQ(4242, configs[0].port);

	removeConfigFile(path);
}

void	testConfigParserBodySize()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"client_max_body_size 10M;\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ((size_t)(10 * 1024 * 1024),
		configs[0].maxBodySize);

	removeConfigFile(path);
}

void	testConfigParserErrorPage()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"error_page 404 errors/404.html;\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ(
		std::string("errors/404.html"),
		configs[0].errorPages[404]);

	removeConfigFile(path);
}

void	testConfigParserLocation()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"location /images {\n"
			"root ./www;\n"
			"autoindex on;\n"
			"}\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ((size_t)1, configs[0].routes.size());
	ASSERT_EQ(std::string("/images"),
		configs[0].routes[0].path);
	ASSERT_EQ(std::string("./www"),
		configs[0].routes[0].root);
	ASSERT_TRUE(configs[0].routes[0].autoindex);

	removeConfigFile(path);
}

void	testConfigParserMethods()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"location / {\n"
			"allow_methods GET POST DELETE;\n"
			"}\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_TRUE(
		configs[0].routes[0].methods.count("GET"));
	ASSERT_TRUE(
		configs[0].routes[0].methods.count("POST"));
	ASSERT_TRUE(
		configs[0].routes[0].methods.count("DELETE"));

	removeConfigFile(path);
}

void	testConfigParserRedirect()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"location /old {\n"
			"return 301 /new;\n"
			"}\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ(301,
		configs[0].routes[0].redirectCode);
	ASSERT_EQ(std::string("/new"),
		configs[0].routes[0].redirectUrl);

	removeConfigFile(path);
}

void	testConfigParserCgi()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"location /cgi {\n"
			"cgi .py /usr/bin/python3;\n"
			"}\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ(
		std::string("/usr/bin/python3"),
		configs[0].routes[0].cgiHandlers[".py"]);

	removeConfigFile(path);
}

void	testConfigParserMultipleServers()
{
	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"}\n"
			"server {\n"
			"listen 9090;\n"
			"}\n");

	ConfigParser			parser(path);
	std::vector<ServerConfig>	configs;

	configs = parser.parse();

	ASSERT_EQ((size_t)2, configs.size());
	ASSERT_EQ(8080, configs[0].port);
	ASSERT_EQ(9090, configs[1].port);

	removeConfigFile(path);
}

void	testConfigParserInvalidPort()
{
	bool	threw = false;

	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 70000;\n"
			"}\n");

	try
	{
		ConfigParser	parser(path);

		parser.parse();
	}
	catch (const std::exception&)
	{
		threw = true;
	}

	ASSERT_TRUE(threw);

	removeConfigFile(path);
}

void	testConfigParserUnknownDirective()
{
	bool	threw = false;

	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"banana test;\n"
			"}\n");

	try
	{
		ConfigParser	parser(path);

		parser.parse();
	}
	catch (const std::exception&)
	{
		threw = true;
	}

	ASSERT_TRUE(threw);

	removeConfigFile(path);
}

void	testConfigParserMissingBrace()
{
	bool	threw = false;

	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n");

	try
	{
		ConfigParser	parser(path);

		parser.parse();
	}
	catch (const std::exception&)
	{
		threw = true;
	}

	ASSERT_TRUE(threw);

	removeConfigFile(path);
}

void	testConfigParserInvalidAutoindex()
{
	bool	threw = false;

	std::string	path =
		createConfigFile(
			"server {\n"
			"listen 8080;\n"
			"location / {\n"
			"autoindex maybe;\n"
			"}\n"
			"}\n");

	try
	{
		ConfigParser	parser(path);

		parser.parse();
	}
	catch (const std::exception&)
	{
		threw = true;
	}

	ASSERT_TRUE(threw);

	removeConfigFile(path);
}

// ------------------------------------------------------- entry point -------------------------------------------------
void	testConfigParser()
{	
	RUN_TEST(testConfigParserSimpleServer);
	RUN_TEST(testConfigParserHostPort);
	RUN_TEST(testConfigParserBodySize);
	RUN_TEST(testConfigParserErrorPage);
	RUN_TEST(testConfigParserLocation);
	RUN_TEST(testConfigParserMethods);
	RUN_TEST(testConfigParserRedirect);
	RUN_TEST(testConfigParserCgi);
	RUN_TEST(testConfigParserMultipleServers);
	RUN_TEST(testConfigParserInvalidPort);
	RUN_TEST(testConfigParserUnknownDirective);
	RUN_TEST(testConfigParserMissingBrace);
	RUN_TEST(testConfigParserInvalidAutoindex);
}
