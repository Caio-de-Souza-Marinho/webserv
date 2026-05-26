#include "../include/WebServer.hpp"
#include "../include/Logger.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Router.hpp"
#include "../include/ResponseBuilder.hpp"
#include <cerrno>

#define	MAX_EVENTS	64
#define	TIMEOUT_MS	5000	// how often checkTimeout() runs, in milliseconds

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

// methods
void	WebServer::run()
{
	struct	epoll_event	events[MAX_EVENTS];

	Logger::info("Server running");

	while (true)
	{
		int	n = epoll_wait(epfd, events, MAX_EVENTS, TIMEOUT_MS);

		if (n == -1)
		{
			if (errno == EINTR)	// interrupted by signal (e.g. SIGCHLD from CGI) - just retry
				continue ;
			Logger::error("epoll_wait failed");
			break ;
		}

		// timeout (n == 0) - no evnets, just run timeout checks
		if (n == 0)
		{
			checkTimeouts();
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

			// error or hangup on a client/CGI fd
			if (ev & (EPOLLERR | EPOLLHUP))
			{
				// if it's a CGI pipe, let handleCGI deal with it (EOF)
				// otherwise close the client
				if (clients.count(fd))
				{
					Client	&client = clients[fd];
					if (client.state == CGI_RUNNING)
						handleCGI(client);
					else
						closeClient(fd);
				}
				else
					closeClient(fd);
				continue ;
			}

			// CGI Pipe events
			// cgiOutputFd and cgiInputFd are not keys in 'clients'
			// we find the owning client by scanning
			{
				bool	isCgiFd = false;
				for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
				{
					Client &client = it->second;
					if (fd == client.cgiOutputFd || fd == client.cgiInputFd)
					{
						handleCGI(client);
						isCgiFd = true;
						break ;
					}
				}
				if (isCgiFd)
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
		client.parser = new RequestParser();

		clients[clientFd] = client;

		Logger::info("Client connected fd=" + intToStr(clientFd));
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
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break ;
			closeClient(fd);
			return ;
		}

		client.readBuffer.append(buffer, bytes);
		client.lastActivity = time(NULL);
	}

	RequestParser::State	state;
	state = client.parser->parse(client.request, client.readBuffer);

	if (state == RequestParser::COMPLETE)
		handleRequest(client);
}

// skipping CGI integration temporarily
void	WebServer::handleRequest(Client &client)
{
	const Route	*route;
	Response	response;

	route = router->matchRoute(client.request, client.server->config);

	if (route && !router->isMethodAllowed(*route, client.request.method))
	{
		response = responseBuilder->buildErrorResponse(405, client.server->config);
	}
	else
	{
		response = responseBuilder->buildResponse(client.request, route, client.server->config);
	}

	client.response = response;
	client.writeBuffer = response.build();
	client.writeOffset = 0;
	client.state = WRITING;

	modifyEpoll(client.fd, EPOLLOUT);
}

void	WebServer::writeClient(int fd)
{
	if (!clients.count(fd))
		return ;

	Client	&client = clients[fd];

	while (client.writeOffset < client.writeBuffer.size())
	{
		ssize_t	sent = send(fd, client.writeBuffer.c_str() + client.writeOffset, client.writeBuffer.size() - client.writeOffset, 0);

		if (sent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return ;
			closeClient(fd);
			return ;
		}

		client.writeOffset += sent;
		client.lastActivity = time(NULL);
	}

	if (!client.request.keepAlive)
	{
		closeClient(fd);
		return ;
	}

	client.readBuffer.clear();
	client.writeBuffer.clear();
	client.writeOffset = 0;
	client.request.reset();
	client.parser->reset();
	client.state = READING;

	modifyEpoll(fd, EPOLLIN);
}

void	WebServer::closeClient(int fd)
{
	std::map<int, Client>::iterator	it = clients.find(fd);

	removeFromEpoll(fd);
	clients.erase(it);
	close(fd);

	if (it == clients.end())
		return ;

	if (it->second.parser)
		delete it->second.parser;

	if (it->second.cgiInputFd != -1)
		close(it->second.cgiInputFd);

	if (it->second.cgiOutputFd != -1)
		close(it->second.cgiOutputFd);
}

void	WebServer::checkTimeouts()
{
	time_t			now = time(NULL);
	std::vector<int>	toClose;

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		Client	&client = it->second;

		if (now - client.lastActivity > 30)
			toClose.push_back(client.fd);
	}

	for (size_t i = 0; i < toClose.size(); i++)
		closeClient(toClose[i]);
}

// only for the compile to work - for now
void	WebServer::handleCGI(Client &client)
{
	(void)client;
}
