/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 17:00:04 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 17:37:28 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Router.hpp"
#include "../include/Config.hpp"
#include "../include/Request.hpp"
#include <cstddef>


Router::Router () {}

// -- Match Router
const Route* Router::matchRoute(const Request &request, const ServerConfig &config) const
{
	return (findBestMatch(request.path, config.routes));
}

// -- Helper findBestMatch
static bool routeMatchesPath(const std::string &routePath, const std::string &requestPath)
{
	if (routePath == "/")
		return (true);
	if (requestPath == routePath)
		return (true);
	if (requestPath.size() > routePath.size()
			&& requestPath.compare(0, routePath.size(), routePath) == 0
			&& requestPath[routePath.size()] == '/')
		return (true);
	return (false);

}

// -- findBestMatch
const Route* Router::findBestMatch(const std::string &path, const std::vector<Route> &routes) const
{
	const Route	*best;
	std::vector<Route>::const_iterator it; 
	size_t	Bestlen;

	best = NULL;
	Bestlen = 0;
	for (it = routes.begin(); it != routes.end(); ++it)
	{
		if (routeMatchesPath(it->path, path) && it->path.size() > Bestlen)
		{
			best = &(*it);
			Bestlen = it->path.size();
		}
	}
	return (best);
}

bool	Router::isMethodAllowed(const Route &route, const std::string &method) const
{
	return (route.methods.find(method) != route.methods.end());
}

// -- matchCGI
// Walk the route's extension->interpreter map. If the request path ends with
// one of the registered extensions (e.g. ".py"), return a pointer to the
// interpreter string for that extension. Otherwise NULL (not a CGI request).
const std::string*	Router::matchCGI(const Route &route, const std::string &path) const
{
	std::map<std::string, std::string>::const_iterator	it;

	for (it = route.cgiHandlers.begin(); it != route.cgiHandlers.end(); ++it)
	{
		const std::string	&ext = it->first;

		if (path.size() >= ext.size()
			&& path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
			return (&it->second);
	}
	return (NULL);
}


std::string	Router::resolvePath(const Route &route, const Request &request) const
{
	std::string relative;

	if (route.path == "/")
		return (route.root + request.path);
	
	relative = request.path.substr(route.path.size());
	return (route.root + relative);
}
