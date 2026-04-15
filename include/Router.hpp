#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include "Config.hpp"
#include "Request.hpp"

class	Router
{
	public:
		Router();

		const Route*	matchRoute(const Request &request, const ServerConfig &config) const;
		bool		isMethodAllowed(const Route &route, const std::string &method) const;
		std::string	resolvePath(const Route &route, const Request &request) const;

	private:
		const Route*	findBestMatch(const std::string &path, const std::vector<Route> &routes) const;
};

#endif
