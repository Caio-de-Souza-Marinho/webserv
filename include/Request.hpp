#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class	Request
{
	public:
		Request();

		std::string				method;
		std::string				rawUri;
		std::string				uri;
		std::string				path;
		std::string				host;
		std::string				query;
		std::string				version;
		std::map<std::string, std::string>	headers;
		std::string				body;
		size_t					contentLength;
		bool					isChunked;
		bool					isComplete;
		bool					keepAlive;

		void	reset();
};

#endif
