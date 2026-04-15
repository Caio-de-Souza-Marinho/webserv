#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class	Request
{
	public:
		std::string				method;
		std::string				uri;
		std::string				path;
		std::string				query;
		std::string				version;
		std::map<std::string, std::string>	headers;
		std::string				body;
		size_t					contentLength;
		bool					isChunked;

		Request() :
			contentLength(0),
			isChunked(false)
		{}
};

#endif
