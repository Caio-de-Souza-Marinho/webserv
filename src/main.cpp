#include "../include/webserv.hpp"

int	main(int argc, char **argv)
{
	std::string	configPath = "config/default.conf";

	if (argc == 2)
		configPath = argv[1];

	WebServer	server(configPath);
	server.run();

	return (0);
}
