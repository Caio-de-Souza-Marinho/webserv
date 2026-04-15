#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <map>
#include <string>

class Server;
class Client;
class RequestParser;
class Router;
class ResponseBuilder;
class CGIHandler;

class	WebServer
{
	public:
		WebServer(const std::string &configPath);
		void run();

	private:
		int				epfd;
		std::vector<Server>		servers;
		std::map<int, Client>		clients;
		std::map<int, Server*>		fdToServer;
		RequestParser			*parser;
		Router				*router;
		ResponseBuilder			*responseBuilder;
		CGIHandler			*cgiHandler;

		void	acceptClient(int serverFd);
		void	readClient(int fd);
		void	writeClient(int fd);
		void	handleRequest(Client &client);
		void	handleCGI(Client &client);
		void	closeClient(int fd);
		void	checkTimeouts();
};

#endif
