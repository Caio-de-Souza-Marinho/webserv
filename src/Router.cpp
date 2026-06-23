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

// -- cgiExtEnd
// Returns the index just past `ext` if `path` contains it at a segment boundary
// (i.e. the extension is followed by end-of-string or '/', as in
// "/cgi-bin/app.py" or "/cgi-bin/app.py/extra"). Otherwise std::string::npos.
static std::string::size_type	cgiExtEnd(const std::string &path, const std::string &ext)
{
	std::string::size_type	pos = path.find(ext);

	while (pos != std::string::npos)
	{
		std::string::size_type	end = pos + ext.size();
		if (end == path.size() || path[end] == '/')
			return (end);
		pos = path.find(ext, pos + 1);
	}
	return (std::string::npos);
}

// -- matchCGI
// Walk the route's extension->interpreter map. If the request path contains one
// of the registered extensions (e.g. ".py") at a segment boundary, return a
// pointer to the interpreter string for that extension. Otherwise NULL.
const std::string*	Router::matchCGI(const Route &route, const std::string &path) const
{
	std::map<std::string, std::string>::const_iterator	it;

	for (it = route.cgiHandlers.begin(); it != route.cgiHandlers.end(); ++it)
	{
		if (cgiExtEnd(path, it->first) != std::string::npos)
			return (&it->second);
	}
	return (NULL);
}

// -- splitCgiPath
// Splits a CGI request path into the script portion and PATH_INFO. For
// "/cgi-bin/app.py/extra/path" with a ".py" handler:
//   scriptName = "/cgi-bin/app.py", pathInfo = "/extra/path".
// When there is no extra component, pathInfo is left empty.
void	Router::splitCgiPath(const Route &route, const std::string &path,
		std::string &scriptName, std::string &pathInfo) const
{
	std::map<std::string, std::string>::const_iterator	it;

	scriptName = path;
	pathInfo.clear();
	for (it = route.cgiHandlers.begin(); it != route.cgiHandlers.end(); ++it)
	{
		std::string::size_type	end = cgiExtEnd(path, it->first);
		if (end != std::string::npos)
		{
			scriptName = path.substr(0, end);
			pathInfo   = path.substr(end);
			return ;
		}
	}
}

std::string	Router::resolvePath(const Route &route, const Request &request) const
{
	return (resolvePath(route, request.path));
}

std::string	Router::resolvePath(const Route &route, const std::string &reqPath) const
{
	std::string relative;

	if (route.path == "/")
		return (route.root + reqPath);

	relative = reqPath.substr(route.path.size());
	return (route.root + relative);
}
