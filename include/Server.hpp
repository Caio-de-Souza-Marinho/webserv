#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"

class	Server
{
	public:
		Server();

		int		fd;
	 	ServerConfig	config;
};

#endif
