#include "../include/WebServer.hpp"
#include "../include/Logger.hpp"
#include "../include/Request.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Config.hpp"
#include "../include/Response.hpp"
#include "../include/Router.hpp"
#include "../include/Colors.hpp"

#include <iostream>
#include <exception>

int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return (1);
	}

	try
	{
		WebServer	server(argv[1]);
		server.run();
	}
	catch (const std::exception &e)
	{
		Logger::error(e.what());
		return (1);
	}

	return (0);
}

/*
int	main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	std::cout << "hello world\n";
	ServerConfig Config = makeMockConfig();
	return (0);
}

void	testRequestParser(void);
void	testResponseBuild(void);
void	testResolvePath(void);
void	testRouterMatch(void);
void	testRouterMethods(void);
void makeTesteBuildResponse(void);

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

	std::cout << GREEN << "Test Router Match: " << RESET << std::endl;
	testRouterMatch();
	std::cout << "----------------------------" << std::endl;

	std::cout << GREEN << "Test Router Methods: " << RESET << std::endl;
	testRouterMethods();
	std::cout << "----------------------------" << std::endl;

	std::cout << GREEN << "Test Response Builder: " << RESET << std::endl;
	makeTesteBuildResponse();
	std::cout << "----------------------------" << std::endl;
	return (0);
}
*/
