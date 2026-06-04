#include "../../include/ResponseBuilder.hpp"
#include <sstream>

std::string	ResponseBuilder::buildDefaultErrorBody(int code) const
{
	std::ostringstream	codeStream;
	std::string		body;
	
	codeStream << code;
	body = "<html><body><h1>";
	body += codeStream.str();
	body += " ";
	body += Response::getStatusMessage(code);
	body += "</h1></body></html>";
	return (body);
}

Response ResponseBuilder::handleError(int statusCode, const ServerConfig &config)
{
	Response	res;
	std::map<int, std::string>::const_iterator it;

	res.statusCode = statusCode;
	res.headers["Content-Type"] = "text/html";

	it = config.errorPages.find(statusCode);
	if (it != config.errorPages.end() && readFile(it->second, res.body))
		return (res);
	res.body = buildDefaultErrorBody(statusCode);
	return (res);
}
