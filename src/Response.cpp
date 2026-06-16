/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 15:53:35 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 16:25:56 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Response.hpp"
#include <ctime>
#include <sstream>

Response::Response() : statusCode(200), keepAlive(false) {}

std::string Response::build() const
{
	std::ostringstream response;
	std::ostringstream length;
	std::map<std::string, std::string> finalHeaders;
	std::map<std::string, std::string>::const_iterator it;

	finalHeaders = headers;
	length << body.size();
	finalHeaders["Content-Length"] = length.str();
	if (finalHeaders.find("Content-Type") == finalHeaders.end() && !body.empty())
		finalHeaders["Content-Type"] = "text/html";
	finalHeaders["Server"] = "webserv/1.0";
	finalHeaders["Date"] = this->getDate();
	finalHeaders["Connection"] = keepAlive ? "keep-alive" : "close";

	response << "HTTP/1.1 " << statusCode << " " << getStatusMessage(statusCode) << "\r\n";

	for (it = finalHeaders.begin(); it != finalHeaders.end(); ++it)
		response << it->first << ": " << it->second << "\r\n";

	response << "\r\n";
	response << body;
	return (response.str());
}

std::string Response::getStatusMessage(int code)
{
	if (code == 200)
		return ("OK");
	if (code == 201)
		return ("Created");
	if (code == 204)
		return ("No Content");
	if (code == 301)
		return ("Moved Permanently");
	if (code == 302)
		return ("Found");
	if (code == 400)
		return ("Bad Request");
	if (code == 401)
		return ("Unauthorized");
	if (code == 403)
		return ("Forbidden");
	if (code == 404)
		return ("Not Found");
	if (code == 405)
		return ("Method Not Allowed");
	if (code == 408)
		return ("Request Timeout");
	if (code == 413)
		return ("Content Too Large");
	if (code == 414)
		return ("URI Too Long");
	if (code == 500)
		return ("Internal Server Error");
	if (code == 501)
		return ("Not Implemented");
	if (code == 502)
		return ("Bad Gateway");
	if (code == 504)
		return ("Gateway Timeout");
	if (code == 505)
		return ("HTTP Version	");
	return ("Unknown");
}

std::string	Response::getDate() const
{
	char	buf[64];
	time_t	now = time(NULL);
	struct std::tm *gmt = gmtime(&now);

	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return (std::string(buf));
}
