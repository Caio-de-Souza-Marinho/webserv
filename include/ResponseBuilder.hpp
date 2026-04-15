#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include <string>
#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"

class	ResponseBuilder
{
	public:
		ResponseBuilder();

		Response	buildResponse(const Request &request, const Route *route, const ServerConfig &config);
		Response	buildErrorResponse(int code, const ServerConfig &config);
		
	private:
		Response	handleGET(const Request &request, const Route &route);
		Response	handlePOST(const Request &request, const Route &route);
		Response	handleDELETE(const Request &request, const Route &route);
		Response	handleRedirect(const Route &route);
		Response	handleError(int statusCode, const ServerConfig &config);
		bool		fileExists(const std::string &path) const;
		std::string	readFile(const std::string &path) const;
		std::string	getContentType(const std::string &path) const;
};

#endif
