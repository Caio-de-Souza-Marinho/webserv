#include "../include/WebServer.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Router.hpp"
#include "../include/ResponseBuilder.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/SessionManager.hpp"
#include "../include/Logger.hpp"
#include "../include/RequestParser.hpp"

WebServer::WebServer(const std::string &configPath)
	: epfd(-1), router(NULL), responseBuilder(NULL), cgiHandler(NULL), sessionManager(NULL)
{
	// Ignore SIGPIPE: writing to a socket/pipe whose other end is closed
	// (a disconnected client, or a CGI that exited) must not kill the server.
	signal(SIGPIPE, SIG_IGN);

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	// 1. config parser
	ConfigParser			parser(configPath);
	std::vector<ServerConfig>	configs = parser.parse();

	// 2. create epoll
	epfd = epoll_create(1);
	if (epfd == -1)
		throw std::runtime_error("epoll_create failed");

	// 3. for each ServerConfig, create a listening socket
	for (size_t i = 0; i < configs.size(); i++)
	{
		Server	server;
		server.config = configs[i];

		// socket TCP
		server.fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server.fd == -1)
			throw std::runtime_error("socket() failed at port " + intToStr(configs[i].port));

		// SO_REUSEADDR - avoid "address already in use" at restart
		int	opt = 1;
		if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			throw std::runtime_error("setsockopt failed");

		setNonBlocking(server.fd);

		// bind
		struct sockaddr_in	addr;
      		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family		= AF_INET;
		addr.sin_port		= htons(static_cast<uint16_t>(configs[i].port));
		addr.sin_addr.s_addr	= inet_addr(configs[i].host.c_str());
		if (addr.sin_addr.s_addr == INADDR_NONE)
			addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(server.fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1)
			throw std::runtime_error("bind() failed at port " + intToStr(configs[i].port));

		// listen - backlog 128 is the default value
		if (listen(server.fd, 128) ==  -1)
			throw std::runtime_error("listen() failed");

		// epoll register
		struct epoll_event	ev;
      		std::memset(&ev, 0, sizeof(ev));
		ev.events	= EPOLLIN;
		ev.data.fd	= server.fd;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, server.fd, &ev) == -1)
			throw std::runtime_error("epoll_ctl ADD server failed");

		// store the server and the mapping fd -> Server*
		servers.push_back(server);
	}

	// 4. Instantiate the helpers (they're stateless, one for each process is enough
	router		= new Router();
	responseBuilder	= new ResponseBuilder();
	cgiHandler	= new CGIHandler();
	sessionManager	= new SessionManager();

	// 5. make the map fd -> Server* now that servers is stable in memory
	for (size_t i = 0; i < servers.size(); i++)
		fdToServer[servers[i].fd] = &servers[i];

	printBanner();
}

WebServer::~WebServer()
{
	// Close all remaining client connections: closeClient() handles the client
	// socket, parser, and any open CGI pipe FDs / zombie processes.
	// Collect fds first to avoid iterator invalidation inside closeClient().
	std::vector<int>	fds;
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		fds.push_back(it->first);
	for (size_t i = 0; i < fds.size(); i++)
		closeClient(fds[i]);

	// close all server sockets
	for (size_t i = 0; i < servers.size(); i++)
		close(servers[i].fd);

	// close epoll fd
	if (epfd != -1)
		close(epfd);

	delete router;
	delete responseBuilder;
	delete cgiHandler;
	delete sessionManager;
}
