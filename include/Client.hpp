#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "Request.hpp"

class	Client
{
	public:
		int		fd;
		std::string	readBuffer;
		std::string	writeBuffer;
		Request		request;
		bool		requestReady;
};

#endif
