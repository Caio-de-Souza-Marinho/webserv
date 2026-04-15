#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"

class	Server
{
	public:
		int		fd;
	 	ServerConfig	config;

		Server() : fd(-1) {}
};

#endif
