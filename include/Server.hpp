#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"

class	Server
{
	public:
		int		fd;	// listeining socket
	 	ServerConfig	config;
};

#endif
