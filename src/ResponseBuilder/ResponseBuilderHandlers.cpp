#include "../../include/ResponseBuilder.hpp"
#include "../../include/Router.hpp"
#include <cctype>
#include <cstddef>
#include <unistd.h>
#include <ctime>


Response 	ResponseBuilder::handleRedirect(const Route &route)
{
	Response	res;

	res.statusCode = route.redirectCode;
	res.headers["Location"] = route.redirectUrl;
	res.body = "";
	return (res);
}

Response	ResponseBuilder::handleGET(const Request &request, const Route &route, const ServerConfig &config)
{
	Router		router;
	std::string	path;

	path = router.resolvePath(route, request);
	if (!fileExists(path))
		return (handleError(404, config));
	if (isDirectory(path))
	{
		return (handleDirectory(path, request.path, route, config));
	}
	if (!isFileReadable(path))
		return (handleError(403, config));
	return (buildSimpleResponse(200, getContentType(path), readFile(path)));
}

// stubs handles
Response	ResponseBuilder::handlePOST(const Request &request,
	const Route &route, const ServerConfig &config)
{
	std::string	fileName;
	std::string	filePath;
	Response	res;

	if (route.uploadPath.empty())
		return (handleError(403, config));
	if (!fileExists(route.uploadPath) || !isDirectory(route.uploadPath))
		return (handleError(500, config));

	fileName = extractUploadFilename(request);
	if (fileName.empty())
		fileName = generateUploadFilename();

	filePath = joinPath(route.uploadPath, fileName);
	if (!writeFile(filePath, request.body))
		return (handleError(500, config));

	res.statusCode = 201;
	res.headers["Content-Type"] = "text/plain";
	res.headers["Location"] = filePath;
	res.body = "File uploaded\n";
	return (res);
}

Response	ResponseBuilder::handleDELETE(const Request &request, const Route &route, const ServerConfig &config)
{
	Router		router;
	std::string	path;

	path = router.resolvePath(route, request);
	if (!fileExists(path))
		return (handleError(404, config));
	if (isDirectory(path))
		return (handleError(403, config));
	if (unlink(path.c_str()) != 0)
		return (handleError(500, config));
	return (buildSimpleResponse(204, "", ""));
}

Response	ResponseBuilder::handleDirectory(const std::string &fsPath, const std::string &urlPath, const Route &route, const ServerConfig &config)
{
	std::string indexPath;
	std::string dirPath;

	dirPath = fsPath;
	if (!dirPath.empty() && dirPath[dirPath.size() - 1] != '/')
		dirPath += "/";
	for (size_t i = 0; i < route.index.size(); i++)
	{
		indexPath = dirPath + route.index[i];
		if (fileExists(indexPath) && !isDirectory(indexPath) && isFileReadable(indexPath))
			return (buildSimpleResponse(200, getContentType(indexPath), readFile(indexPath)));
	}
	if (route.autoindex)
		return (buildSimpleResponse(200, "text/html", generateAutoindex(dirPath, urlPath)));
	return (handleError(403, config));
}
