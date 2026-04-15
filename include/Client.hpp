#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

#include "Request.hpp"
#include "Response.hpp"

class Server;
class RequestParser;

enum	ClientState
{
	READING,
	WRITING,
	CGI_RUNNING
};

class	Client
{
	public:
		Client();

		int		fd;
		std::string	readBuffer;
		std::string	writeBuffer;
		size_t		writeOffset;
		Request		request;
		Response	response;
		ClientState	state;
		bool		requestComplete;
		time_t		lastActivity;
		Server		*server;
		RequestParser	*parser;
		pid_t		cgiPid;
		int		cgiInputFd;
		int		cgiOutputFd;
		std::string	cgiBuffer;
		bool		cgiDone;
};

#endif
