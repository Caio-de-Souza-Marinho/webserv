#include "../../include/ResponseBuilder.hpp"
#include "../../include/Router.hpp"

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
