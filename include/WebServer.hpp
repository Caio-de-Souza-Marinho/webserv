#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <cerrno>

#include "Client.hpp"
#include "Server.hpp"
#include "SessionManager.hpp"

class Router;
class ResponseBuilder;
class CGIHandler;
class SessionManager;

extern volatile sig_atomic_t	g_running;
void	signalHandler(int);

class	WebServer
{
	public:
		WebServer(const std::string &configPath);
		~WebServer();
		void run();

	private:
		int				epfd;
		std::vector<Server>		servers;
		std::map<int, Client>		clients;
		std::map<int, Server*>		fdToServer;
		Router				*router;
		ResponseBuilder			*responseBuilder;
		CGIHandler			*cgiHandler;
		SessionManager			*sessionManager;

		void		acceptClient(int serverFd);
		void		readClient(int fd);
		void		writeClient(int fd);
		void		handleRequest(Client &client);
		void		handleCGI(Client &client);
		void		registerCgi(Client &client);
		void		finishCgi(Client &client);
		void		handleCgiTimeout(Client &client);
		Response	parseCgiOutput(const std::string &raw);
		void		closeClient(int fd);
		void		checkTimeouts();
		void		modifyEpoll(int fd, uint32_t events);
		void		removeFromEpoll(int fd);
		void		setNonBlocking(int fd);
		std::string	intToStr(int n);
		void		prepareResponse(Client &client, const Response &response);
		void		printBanner(void);
		void		logAccess(const Client &client);	
		void		handleSessionTest(Client &client);
};

#endif
