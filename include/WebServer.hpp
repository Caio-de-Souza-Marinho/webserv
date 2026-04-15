#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Server.hpp"
#include "Client.hpp"

class	WebServer
{
	private:
		int				epfd;
		std::vector<Server>		servers;
		std::map<int, Client>		clients;
		std::vector<ServerConfig>	configs;

		void	acceptClilent(int serverFd);
		void	readClient(int fd);
		void	writeClient(int fd);
		void	handleRequest(Client &client);
		void	closeClient(int fd);

	public:
		WebServer(const std::string &configPath);
		void run();
};

#endif
