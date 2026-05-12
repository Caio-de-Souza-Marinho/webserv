#include "../include/WebServer.hpp"
#include "../include/Request.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Config.hpp"
#include "../include/Response.hpp"
#include "../include/Router.hpp"
#include "../include/Colors.hpp"
#include <iostream>

void	testRequestParser(void);
void	testResponseBuild(void);
void	testResolvePath(void);
void	testRouterMatch(void);
void	testRouterMethods(void);
/*
int	main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	std::cout << "hello world\n";
	ServerConfig Config = makeMockConfig();
	return (0);
}
*/

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

	std::cout << GREEN << "Test Resolve Path: " << RESET << std::endl;
	testResolvePath();
	std::cout << "----------------------------" << std::endl;

	std::cout << GREEN << "Test Router Match: " << RESET << std::endl;
	testRouterMatch();
	std::cout << "----------------------------" << std::endl;

	std::cout << GREEN << "Test Router Methods: " << RESET << std::endl;
	testRouterMethods();
	std::cout << "----------------------------" << std::endl;
	return (0);
}

