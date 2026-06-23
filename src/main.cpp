#include "../include/WebServer.hpp"
#include "../include/Logger.hpp"
#include <exception>

int	main(int argc, char **argv)
{
	try
	{
		std::string	configPath = (argc >= 2) ? argv[1] : "config/42.conf";
		WebServer	server(configPath);
		server.run();
	}
	catch (const std::exception &e)
	{
		Logger::error(e.what());
		return (1);
	}

	return (0);
}
