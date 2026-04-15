#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class	Response
{
	public:
		int					statusCode;
		std::string				contentType;
		std::map<std::string, std::string>	headers;
		std::string				body;

		std::string	build();	// assembles the raw HTTP response string
		
};

#endif
