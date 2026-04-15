#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class	Response
{
	public:
		Response();

		int					statusCode;
		std::map<std::string, std::string>	headers;
		std::string				body;

		std::string		build() const;
		static	std::string	getStatusMessage(int code);
};

#endif
