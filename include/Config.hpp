#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct	Route
{
	std::string				path;
	std::string				root;
	std::string				index;
	std::vector<std::string>		methods;
	std::string				redirectUrl;
	int					redirectCode;
	bool					autoindex;
	std::string				uploadPath;
	std::map<std::string, std::string>	cgiHandlers;

	Route() :
		redirectCode(0),
		autoindex(false)
	{}
};

struct	ServerConfig
{
	std::string			host;
	int				port;
	size_t				maxBodySize;
	std::map<int, std::string>	errorPages;
	std::vector<Route>		routes;

	ServerConfig() :
		port(0),
		maxBodySize(0)
	{}
};

#endif
