#include "../include/WebServer.hpp"
#include "../include/ResponseBuilder.hpp"
#include "../include/Logger.hpp"
#include <cerrno>
#include <cstdlib>

// Add the two CGI pipe fds to epoll so the main loop wakes up on them.
// - cgiOutputFd: we always want to READ the script's output  -> EPOLLIN
// - cgiInputFd : we only need to WRITE if there is a body     -> EPOLLOUT
//   (no body -> close it right away so the script sees EOF on stdin)
void	WebServer::registerCgi(Client &client)
{
	struct epoll_event	ev;

	std::memset(&ev, 0, sizeof(ev));
	ev.events  = EPOLLIN;
	ev.data.fd = client.cgiOutputFd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, client.cgiOutputFd, &ev) == -1)
		Logger::error("epoll_ctl ADD cgiOutputFd failed");

	if (!client.request.body.empty())
	{
		std::memset(&ev, 0, sizeof(ev));
		ev.events  = EPOLLOUT;
		ev.data.fd = client.cgiInputFd;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, client.cgiInputFd, &ev) == -1)
			Logger::error("epoll_ctl ADD cgiInputFd failed");
	}
	else
	{
		close(client.cgiInputFd);
		client.cgiInputFd = -1;
	}
}

// Called by run() whenever one of the client's CGI pipes is ready.
// We don't know which fd/event fired, so we just try both operations;
// the fd that isn't ready returns EAGAIN, which we ignore.
void	WebServer::handleCGI(Client &client)
{
	// 1) push the request body into the script's stdin (if still pending)
	if (client.cgiInputFd != -1)
	{
		while (!client.request.body.empty())
		{
			ssize_t	w = write(client.cgiInputFd,
					client.request.body.c_str(),
					client.request.body.size());
			if (w > 0)
				client.request.body.erase(0, w);
			else
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
					break ;			// pipe full, retry on next EPOLLOUT
				// script closed its stdin (EPIPE) or other error: stop writing
				client.request.body.clear();
			}
		}
		if (client.request.body.empty())
		{
			removeFromEpoll(client.cgiInputFd);
			close(client.cgiInputFd);
			client.cgiInputFd = -1;
		}
	}

	// 2) drain whatever the script has written to its stdout
	if (client.cgiOutputFd != -1)
	{
		char	buf[8192];

		while (true)
		{
			ssize_t	r = read(client.cgiOutputFd, buf, sizeof(buf));
			if (r > 0)
				client.cgiBuffer.append(buf, r);
			else if (r == 0)		// EOF: the script finished
			{
				finishCgi(client);
				return ;
			}
			else
				break ;			// EAGAIN: nothing more for now
		}
	}

	client.lastActivity = time(NULL);
}

// The script closed its stdout. Reap it, decide success/failure, build the
// HTTP response and flip the client over to WRITING.
void	WebServer::finishCgi(Client &client)
{
	int	status = 0;

	if (client.cgiOutputFd != -1)
	{
		removeFromEpoll(client.cgiOutputFd);
		close(client.cgiOutputFd);
		client.cgiOutputFd = -1;
	}
	if (client.cgiInputFd != -1)
	{
		removeFromEpoll(client.cgiInputFd);
		close(client.cgiInputFd);
		client.cgiInputFd = -1;
	}

	waitpid(client.cgiPid, &status, 0);
	client.cgiPid = -1;

	Response	response;
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		response = parseCgiOutput(client.cgiBuffer);
	else
		response = responseBuilder->buildErrorResponse(502, client.server->config);

	client.response	   = response;
	client.writeBuffer = response.build();
	client.writeOffset = 0;
	client.cgiBuffer.clear();
	client.state	   = WRITING;

	modifyEpoll(client.fd, EPOLLOUT);
}

// Splits the raw CGI output into headers + body. The script's own headers
// (Content-Type, etc.) are copied into the Response; a "Status: NNN" header
// overrides the status code. Everything after the blank line is the body.
Response	WebServer::parseCgiOutput(const std::string &raw)
{
	Response		res;
	std::string		headerPart;
	std::string		body;
	std::string::size_type	sep;
	size_t			sepLen = 4;

	res.statusCode = 200;

	sep = raw.find("\r\n\r\n");
	if (sep == std::string::npos)
	{
		sep = raw.find("\n\n");
		sepLen = 2;
	}
	if (sep == std::string::npos)
	{
		// no header block at all: treat the whole thing as the body
		res.body = raw;
		res.headers["Content-Type"] = "text/html";
		return (res);
	}

	headerPart = raw.substr(0, sep);
	body       = raw.substr(sep + sepLen);
	res.body   = body;

	// parse header lines one by one
	std::string::size_type	start = 0;
	while (start < headerPart.size())
	{
		std::string::size_type	eol = headerPart.find('\n', start);
		std::string		line;

		if (eol == std::string::npos)
		{
			line = headerPart.substr(start);
			start = headerPart.size();
		}
		else
		{
			line = headerPart.substr(start, eol - start);
			start = eol + 1;
		}
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue ;

		std::string::size_type	colon = line.find(':');
		if (colon == std::string::npos)
			continue ;

		std::string	key = line.substr(0, colon);
		std::string	val = line.substr(colon + 1);
		while (!val.empty() && (val[0] == ' ' || val[0] == '\t'))
			val.erase(0, 1);

		if (key == "Status")
			res.statusCode = std::atoi(val.c_str());
		else
			res.headers[key] = val;
	}

	if (res.headers.find("Content-Type") == res.headers.end())
		res.headers["Content-Type"] = "text/html";
	return (res);
}

// A CGI that ran past its time budget: kill it, reap it, answer 504.
void	WebServer::handleCgiTimeout(Client &client)
{
	Logger::warning("CGI timeout, killing child");

	if (client.cgiPid != -1)
	{
		kill(client.cgiPid, SIGKILL);
		waitpid(client.cgiPid, NULL, 0);
		client.cgiPid = -1;
	}
	if (client.cgiInputFd != -1)
	{
		removeFromEpoll(client.cgiInputFd);
		close(client.cgiInputFd);
		client.cgiInputFd = -1;
	}
	if (client.cgiOutputFd != -1)
	{
		removeFromEpoll(client.cgiOutputFd);
		close(client.cgiOutputFd);
		client.cgiOutputFd = -1;
	}

	Response	response = responseBuilder->buildErrorResponse(504, client.server->config);
	client.response	   = response;
	client.writeBuffer = response.build();
	client.writeOffset = 0;
	client.cgiBuffer.clear();
	client.state	   = WRITING;
	client.lastActivity = time(NULL);

	modifyEpoll(client.fd, EPOLLOUT);
}
