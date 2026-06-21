#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include <string>
#include <sys/stat.h>
#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"
#include "MultipartParser.hpp"

class	ResponseBuilder
{
	public:
		ResponseBuilder();

		Response	buildResponse(const Request &request, const Route *route, const ServerConfig &config);
		Response	buildErrorResponse(int code, const ServerConfig &config);
		
	private:
		Response	handleGET(const Request &request, const Route &route, const ServerConfig &config);
		Response	handlePOST(const Request &request, const Route &route, const ServerConfig &config);
		Response	handleDELETE(const Request &request, const Route &route, const ServerConfig &config);
		Response	handleDirectory(const std::string &fsPath, const std::string &urlPath, const Route &route, const ServerConfig &config);
		Response	handleRedirect(const Route &route);
		Response	handleError(int statusCode, const ServerConfig &config);
		bool		fileExists(const std::string &path) const;
		bool		readFile(const std::string &path, std::string &out) const;
		std::string	getContentType(const std::string &path) const;
		bool		isDirectory(const std::string &path) const;
		std::string	generateAutoindex(const std::string &fsPath, const std::string &urlPath) const;
		std::string	buildDefaultErrorBody(int statusCode) const;
		Response	buildSimpleResponse(int statusCode, const std::string &contentType, const std::string &body) const;
		std::string	joinPath(const std::string &dir, const std::string &file) const;
		std::string	generateUploadFilename(void) const;
		std::string	extractUploadFilename(const Request &request) const;
		bool		writeFile(const std::string &path, const std::string &content) const;
		bool		isSafeFilename(const std::string &name) const;
		bool		isParentWritable(const std::string &path) const;
		std::string	extractParentPath(const std::string &path) const;
		Response	handleMultipartPost(const Request &request, const Route &route, const ServerConfig &config);
		Response	handleRawPost(const Request &request, const Route &route, const ServerConfig &config);
};

#endif
