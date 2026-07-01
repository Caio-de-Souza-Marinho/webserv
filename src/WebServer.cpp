#include "../include/WebServer.hpp"
#include "../include/Logger.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Router.hpp"
#include "../include/ResponseBuilder.hpp"
#include "../include/CGIHandler.hpp"
#include <sstream>

volatile	sig_atomic_t	g_running = 1;

void	signalHandler(int)
{
	g_running = 0;
}

#define	MAX_EVENTS	64
#define TIMEOUT_MS	5000	// how often checkTimeout() runs, in milliseconds
#define MAX_CLIENTS	1024

// helpers
void	WebServer::modifyEpoll(int fd, uint32_t events)
{
	struct epoll_event	ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
		Logger::error("epoll_ctl MOD failed");
}

void	WebServer::logAccess(const Client &client)
{
	std::ostringstream	oss;
	int			status;

	status = client.response.statusCode;

	oss	<< client.ip
		<< " \""
		<< client.request.method
		<< " "
		<< client.request.rawUri
		<< " "
		<< client.request.version
		<< "\" "
		<< status
		<< " "
		<< client.response.body.size();

	if (status >= 500)
		Logger::error(oss.str());
	else if (status >= 400)
		Logger::warning(oss.str());
	else
		Logger::info(oss.str());
}

void	WebServer::prepareResponse(Client &client, const Response &response)
{

	// Copy the response into the client
	client.response = response;

	logAccess(client);

	// Set Connection header according to keep-alive flag
	if (client.request.keepAlive)
		client.response.headers["Connection"] = "keep-alive";
	else
		client.response.headers["Connection"] = "close";

	// Build the wire format and set up writing
	client.writeBuffer = client.response.build();
	client.writeOffset = 0;
	client.state = WRITING;
	modifyEpoll(client.fd, EPOLLOUT);
}

void	WebServer::removeFromEpoll(int fd)
{
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
		Logger::error("epoll_ctl DEL failed");
}

void	WebServer::setNonBlocking(int fd)
{
	int	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl non-blocking failed");
}

std::string	WebServer::intToStr(int n)
{
	std::ostringstream	ss;
	ss << n;
	return (ss.str());
}

void	WebServer::printBanner(void)
{
	std::ostringstream	oss;

	oss << "\n";
	oss << "========================================\n";
	oss << "               webserv\n";
	oss << "========================================\n";
	oss << " Event system	: epoll\n";
	oss << " Servers	: " << servers.size() << "\n\n";

	for (size_t i = 0; i < servers.size(); i++)
	{
		const ServerConfig	&config = servers[i].config;

		oss << " [" << config.port << "] " << config.host << ":" << config.port << "\n";

		for (size_t j = 0; j < config.routes.size(); j++)
		{
			const Route	&route = config.routes[j];

			oss << "   " << route.path << " [";

			for (std::set<std::string>::const_iterator it = route.methods.begin(); it != route.methods.end(); ++it)
			{
				if (it != route.methods.begin())
					oss << ",";
				oss << *it;
			}

			oss << "]";

			if (!route.redirectUrl.empty())
				oss << " -> " << route.redirectCode << " " << route.redirectUrl;

			if (!route.cgiHandlers.empty())
				oss << " [CGI]";
			
			if (!route.uploadPath.empty())
				oss << " [UPLOAD]";

			if (route.autoindex)
				oss << " [AUTOINDEX]";

			oss << "\n";
		}
		oss << "\n";
	}

	oss << " Ready to accept connections\n";
	oss << "========================================\n";

	Logger::info(oss.str());
}

// methods
void	WebServer::run()
{
	struct	epoll_event	events[MAX_EVENTS];

	while (g_running)
	{
		while (true)
		{
			if (!g_running)
				break;
			int	n = epoll_wait(epfd, events, MAX_EVENTS, TIMEOUT_MS);

			if (n == -1)
			{
				if (errno == EINTR)	// interrupted by signal (e.g. SIGCHLD from CGI) - just retry
				{
					if (!g_running)
						break ;
					continue ;
				}
				Logger::error("epoll_wait failed");
				break ;
			}

			// timeout (n == 0) - no evnets, just run timeout checks
			if (n == 0)
			{
				checkTimeouts();
				if (!g_running)
					break;
				continue ;
			}

			for (int i = 0; i < n; i++)
			{
				int		fd = events[i].data.fd;
				uint32_t	ev = events[i].events;

				// is this a server socket? -> new connection
				if (fdToServer.count(fd))
				{
					acceptClient(fd);
					continue ;
				}

				// CGI pipe event? cgiOutputFd / cgiInputFd are not keys in 'clients',
				// so we find the owning client by scanning. This MUST come before the
				// generic EPOLLHUP handling below: a finished CGI hangs up its pipe,
				// and we want handleCGI() to read the final bytes / detect EOF.
				{
					bool	isCgiFd = false;
					for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
					{
						Client &client = it->second;
						if (fd == client.cgiOutputFd)
	  					{
	  						handleCGIRead(client);
	  						isCgiFd = true;
	  						break ;
	  					}

						if (fd == client.cgiInputFd)
	  					{
	  						handleCGIWrite(client);
	  						isCgiFd = true;
	  						break ;
	  					}
					}
					if (isCgiFd)
						continue ;
				}

				// error or hangup on a regular client fd
				if (ev & (EPOLLERR | EPOLLHUP))
				{
					closeClient(fd);
					continue ;
				}

				// regular client fd
				if (!clients.count(fd))
					continue ;

				if (ev & EPOLLIN)
					readClient(fd);
				if (ev & EPOLLOUT)
					writeClient(fd);
			}
			checkTimeouts();
		}
	}
}

void	WebServer::acceptClient(int serverFd)
{
	struct sockaddr_in	addr;
	socklen_t		addrLen = sizeof(addr);

	while (true)
	{
		int	clientFd = accept(serverFd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen);

		if (clientFd == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break ;
			Logger::error("accept() failed");
			return ;
		}

		if (clients.size() >= MAX_CLIENTS)
		{
			Logger::warning("Max clients reached, refusing connection");
			close(clientFd);
			continue ;
		}

		try
		{
			setNonBlocking(clientFd);
		}
		catch (const std::exception &e)
		{
			close(clientFd);
			continue ;
		}

		struct epoll_event	ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN;
		ev.data.fd = clientFd;

		if (epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &ev) == -1)
		{
			Logger::error("epoll_ctl ADD client failed");
			close(clientFd);
			continue ;
		}

		Client	client;
		client.fd = clientFd;
		client.server = fdToServer[serverFd];
		client.lastActivity = time(NULL);
		client.state = READING;
		client.ip = inet_ntoa(addr.sin_addr);
		client.port = ntohs(addr.sin_port);
		client.parser = new RequestParser();

		clients[clientFd] = client;

		std::ostringstream	oss;

		oss	<< "[CONNECT] fd=" 
			<< clientFd 
			<< " ip=" 
			<< client.ip 
			<< ":"
			<< client.port;

		Logger::info(oss.str());
	}
}

void	WebServer::readClient(int fd)
{
	char	buffer[8192];
	ssize_t	bytes;
	Client	&client	= clients[fd];

	while (true)
	{
		bytes = recv(fd, buffer, sizeof(buffer), 0);

		if (bytes == 0)
		{
			closeClient(fd);
			return ;
		}

		if (bytes < 0)
		{
			break ;
		}

		client.readBuffer.append(buffer, bytes);
		client.lastActivity = time(NULL);

		if (client.server->config.maxBodySize > 0 && client.readBuffer.size() > client.server->config.maxBodySize + 8192)
		{
			client.request.keepAlive = false;
			Response	response = responseBuilder->buildErrorResponse(413, client.server->config);
			prepareResponse(client, response);
			return ;
		}
	}

	RequestParser::State	state;
	state = client.parser->parse(client.request, client.readBuffer, client.server->config.maxBodySize);

	if (state == RequestParser::PARSE_ERROR)
	{
		client.request.keepAlive = false;
		Response	response = responseBuilder->buildErrorResponse(
			client.request.errorCode ? client.request.errorCode : 400,
			client.server->config);
		prepareResponse(client, response);
		return ;
	}

	if (state == RequestParser::COMPLETE)
		handleRequest(client);
}

void	WebServer::handleSessionTest(Client &client)
{
	std::string						cookieHeader;
	std::map<std::string, std::string>::const_iterator	it;

	it = client.request.headers.find("cookie");
	if (it != client.request.headers.end())
		cookieHeader = it->second;

	bool	isNew = false;
	std::string	sessionId = sessionManager->resolveSession(cookieHeader, isNew);
	int		visits = sessionManager->incrementVisits(sessionId);

	std::ostringstream	body;
	body << "<html><body><h1>Session Test</h1>"
		<< "<p>Visits in this session: " << visits << "</p>"
		<< "<p>Session ID: " << sessionId << "</p>"
		<< "</body></html>";
	Response	res;
	res.statusCode = 200;
	res.headers["Content-type"] = "text/html";
	if (isNew)
		res.headers["Set-Cookie"] = "session_id=" + sessionId + "; Path=/";
	res.body = body.str();

	prepareResponse(client, res);
}

void	WebServer::handleRequest(Client &client)
{
	const Route	*route;
	Response	response;

	if (client.request.path == "/session-test")
	{
		handleSessionTest(client);
		return ;
	}
	route = router->matchRoute(client.request, client.server->config);

	if (route && !router->isMethodAllowed(*route, client.request.method))
	{
		response = responseBuilder->buildErrorResponse(405, client.server->config);
	}
	else if (route)
	{
		size_t routeLimit = route->maxBodySize;
		if (routeLimit > 0 && client.request.body.size() > routeLimit)
		{
			client.request.keepAlive = false;
			response = responseBuilder->buildErrorResponse(413, client.server->config);
			prepareResponse(client, response);
			return ;
		}

		const std::string	*interpreter = router->matchCGI(*route, client.request.path);
		if (interpreter)
		{
			std::string	scriptName;
			std::string	pathInfo;

			router->splitCgiPath(*route, client.request.path, scriptName, pathInfo);
			std::string	scriptPath = router->resolvePath(*route, scriptName);

			if (cgiHandler->execute(client, scriptPath, *interpreter, scriptName, pathInfo))
			{
				registerCgi(client);
				return ;
			}
			else
				response = responseBuilder->buildErrorResponse(500, client.server->config);
		}
		else
			response = responseBuilder->buildResponse(client.request, route, client.server->config);
	}
	else
	{
		response = responseBuilder->buildResponse(client.request, route, client.server->config);
	}

	prepareResponse(client, response);
}

void	WebServer::writeClient(int fd)
{
	if (!clients.count(fd))
		return ;

	Client	&client = clients[fd];

	ssize_t	sent = send(fd, client.writeBuffer.c_str() + client.writeOffset, client.writeBuffer.size() - client.writeOffset, 0);

	if (sent <= 0)
	{
		closeClient(fd);
		return ;
	}

	client.writeOffset += sent;
	client.lastActivity = time(NULL);

	if (client.writeOffset < client.writeBuffer.size())
		return ;

	if (!client.request.keepAlive)
	{
		closeClient(fd);
		return ;
	}

	client.writeBuffer.clear();
	client.writeOffset = 0;
	client.request.reset();
	client.parser->reset();
	client.state = READING;

	modifyEpoll(fd, EPOLLIN);

	if (!client.readBuffer.empty())
	{
		RequestParser::State state = client.parser->parse(
		client.request, client.readBuffer,
		client.server->config.maxBodySize);

		if (state == RequestParser::PARSE_ERROR)
		{
			client.request.keepAlive = false;
			Response response = responseBuilder->buildErrorResponse(
				client.request.errorCode ? client.request.errorCode : 400,
				client.server->config);
			prepareResponse(client, response);
		}
		else if (state == RequestParser::COMPLETE)
		{
			handleRequest(client);
		}
	}
}

void	WebServer::closeClient(int fd)
{
	std::map<int, Client>::iterator	it = clients.find(fd);
	if (it == clients.end())
		return ;

	Client	&client = it->second;

	// clean up everything *inside* the Client while the iterator is still valid
	if (client.parser)
		delete client.parser;

	// if a CGI was still running, reap the child so it doesn't become a zombie
	if (client.cgiPid != -1)
	{
		kill(client.cgiPid, SIGKILL);
		waitpid(client.cgiPid, NULL, 0);
	}
	if (client.cgiInputFd != -1)
	{
		removeFromEpoll(client.cgiInputFd);
		close(client.cgiInputFd);
	}
	if (client.cgiOutputFd != -1)
	{
		removeFromEpoll(client.cgiOutputFd);
		close(client.cgiOutputFd);
	}

	std::ostringstream	oss;

	oss	<< "[DISCONNECT] fd=" 
		<< fd
		<< " ip=" 
		<< client.ip 
		<< ":"
		<< client.port;

	Logger::info(oss.str());

	removeFromEpoll(fd);
	close(fd);
	clients.erase(it);	// erase last: the iterator is dead afterwards
}

void	WebServer::checkTimeouts()
{
	time_t			now = time(NULL);

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		Client	&client = it->second;

		// A CGI that runs too long gets killed and answered with 504.
		if (client.state == CGI_RUNNING)
		{
			if (now - client.lastActivity > 60)
				handleCgiTimeout(client);
			continue ;
		}

		if (client.state == WRITING)
			continue ;

		if (now - client.lastActivity > 30)
		{
			client.request.keepAlive = false;
			Response	response = responseBuilder->buildErrorResponse(408, client.server->config);
			prepareResponse(client, response);
		}
	}
}
