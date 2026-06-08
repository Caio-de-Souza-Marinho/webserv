#include "../../include/ResponseBuilder.hpp"
#include "../../include/MimeTypes.hpp"
#include <cstddef>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <sstream>
#include <ctime>
#include <unistd.h>

std::string	ResponseBuilder::joinPath(const std::string &dir, const std::string &file) const
{
	if (dir.empty())
		return (file);
	if (dir[dir.size() - 1] == '/')
		return (dir + file);
	return (dir + "/" + file);
}

bool	ResponseBuilder::isSafeFilename(const std::string &name) const
{
	if (name.find("/") != std::string::npos || name.find("..") != std::string::npos)
		return (false);
	return (true);
}

std::string	ResponseBuilder::extractUploadFilename(const Request &request) const
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string	value;
	std::string	filename;
	size_t		pos;
	size_t		end;

	it = request.headers.find("content-disposition");
	if (it == request.headers.end())
		return ("");
	value = it->second;
	pos = value.find("filename=");
	if (pos == std::string::npos)
		return ("");
	pos += 9;
	if (pos < value.size() && value[pos] == '"')
	{
		pos++;
		end = value.find('"', pos);
	}
	else
		end = value.find(';', pos);
	if (end == std::string::npos)
		filename = value.substr(pos);
	else
		filename = value.substr(pos, end - pos);
	return (filename);
}

std::string	ResponseBuilder::generateUploadFilename(void) const
{
	std::ostringstream	name;

	name << "upload_" << time(NULL) << ".dat";
	return (name.str());
}

std::string 	ResponseBuilder::getContentType(const std::string &path) const
{
	return (MimeTypes::getType(path));
}
