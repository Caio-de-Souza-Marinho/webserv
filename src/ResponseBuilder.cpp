/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 18:09:16 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 19:04:56 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ResponseBuilder.hpp"
#include "../include/Router.hpp"
#include "../include/MimeTypes.hpp"
#include <fstream>
#include <sstream>


ResponseBuilder::ResponseBuilder() {};

Response ResponseBuilder::buildResponse(const Request &request, 
		const Route *route, const ServerConfig &config)
{
	if (route == NULL)
		return (handleError(404, config));

	if (route->redirectCode != 0)
		return (handleRedirect(*route));

	if (route->methods.find(request.method) == route->methods.end())
		return (handleError(405, config));

	if (request.method == "GET")
		return (handleGET(request, *route));

	if (request.method == "POST")
		return (handlePOST(request, *route));

	if (request.method == "DELETE")
		return (handleDELETE(request, *route));

	return (handleError(501, config));
}


Response ResponseBuilder::buildErrorResponse(int code, const ServerConfig &config)
{
	return (handleError(code, config));
}

// handles
Response ResponseBuilder::handleError(int statusCode, const ServerConfig &config)
{
	Response	res;
	std::map<int, std::string>::const_iterator it;

	res.statusCode = statusCode;
	res.headers["Content-Type"] = "text/html";

	it = config.errorPages.find(statusCode);
	if (it != config.errorPages.end() && fileExists(it->second))
	{
		res.body = readFile(it->second);
		return (res);
	}
	res.body = "<html><body><h1>";
	res.body += Response::getStatusMessage(statusCode);
	res.body += "</h1></body></html>";
	return (res);
}

Response ResponseBuilder::handleRedirect(const Route &route)
{
	Response	res;

	res.statusCode = route.redirectCode;
	res.headers["Location"] = route.redirectUrl;
	res.body = "";
	return (res);
}

Response	ResponseBuilder::handleGET(const Request &request, const Route &route)
{
	Response	res;
	Router	router;
	std::string	path;

	path = router.resolvePath(route, request);
	if (!fileExists(path))
	{
		res.statusCode = 404;
		res.headers["Content-Type"] = "text/html";
		res.body = "<html><body><h1>404 Not Found</h1></body></html>";
		return (res);
	}
	if (isDirectory(path))
	{
		res.statusCode = 403;
		res.headers["Content-Type"] = "text/html";
		res.body = "<html><body><h1>403 Forbidden</h1></body></html>";
		return (res);
	}
	res.statusCode = 200;
	res.headers["Content-Type"] = getContentType(path);
	res.body = readFile(path);
	return (res);
}

// stubs handles
Response	ResponseBuilder::handlePOST(const Request &request,
	const Route &route)
{
	(void)request;
	(void)route;

	Response	res;

	res.statusCode = 501;
	res.headers["Content-Type"] = "text/html";
	res.body = "<html><body><h1>501 Not Implemented</h1></body></html>";
	return (res);
}

Response	ResponseBuilder::handleDELETE(const Request &request,
	const Route &route)
{
	(void)request;
	(void)route;

	Response	res;

	res.statusCode = 501;
	res.headers["Content-Type"] = "text/html";
	res.body = "<html><body><h1>501 Not Implemented</h1></body></html>";
	return (res);
}

// Helpers
bool ResponseBuilder::fileExists(const std::string &path) const
{
	struct stat info;

	return (stat(path.c_str(), &info) == 0);
}

std::string ResponseBuilder::readFile(const std::string &path) const
{
	std::ifstream		file;
	std::ostringstream	buffer;

	file.open(path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return ("");
	buffer << file.rdbuf();
	file.close();
	return (buffer.str());
}

bool	ResponseBuilder::isDirectory(const std::string &path) const
{
	struct stat	info;

	if (stat(path.c_str(), &info) != 0)
		return (false);
	return (S_ISDIR(info.st_mode));
}

std::string ResponseBuilder::getContentType(const std::string &path) const
{
	return (MimeTypes::getType(path));
}

