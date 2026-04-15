#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class	Request
{
	private:
		std::string				method;
		std::string				uri;
		std::string				path;
		std::string				query;
		std::string				version;
		std::map<std::string, std::string>	headers;
		std::string				body;
		bool					isComplete;
};

#endif
