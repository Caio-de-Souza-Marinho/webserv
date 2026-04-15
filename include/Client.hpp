#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

#include "Request.hpp"
#include "Response.hpp"

class Server;

enum	ClientState
{
	READING,
	WRITING,
	CGI_RUNNING
};

class	Client
{
	public:
		int		fd;
		std::string	readBuffer;
		std::string	writeBuffer;
		size_t		writeOffset;
		Request		request;
		Response	response;
		ClientState	state;
		bool		requestComplete;
		time_t		lastActivity;
		Server*		server;
		pid_t		cgiPid;
		int		cgiFd;

		Client();
		/*
		Client() :
			fd(-1),
			writeOffset(0),
			state(READING),
			requestComplete(false),
			lastActivity(0),
			server(NULL),
			cgiPid(-1),
			cgiFd(-1)
		{}
		*/
};

#endif
