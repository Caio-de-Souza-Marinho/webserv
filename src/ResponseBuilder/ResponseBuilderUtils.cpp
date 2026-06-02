#include "../../include/ResponseBuilder.hpp"
#include "../../include/MimeTypes.hpp"
#include <cstddef>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <unistd.h>

// Helpers
bool	 ResponseBuilder::fileExists(const std::string &path) const
{
	struct stat info;

	return (stat(path.c_str(), &info) == 0);
}

bool 	ResponseBuilder::readFile(const std::string &path, std::string &out) const
{
	std::ifstream		file;
	std::ostringstream	buffer;

	file.open(path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return (false);
	buffer << file.rdbuf();
	file.close();
	out = buffer.str();
	return (true);
}

bool	ResponseBuilder::isDirectory(const std::string &path) const
{
	struct stat	info;

	if (stat(path.c_str(), &info) != 0)
		return (false);
	return (S_ISDIR(info.st_mode));
}

std::string 	ResponseBuilder::getContentType(const std::string &path) const
{
	return (MimeTypes::getType(path));
}


Response	ResponseBuilder::buildSimpleResponse(int statusCode, const std::string &contentType, const std::string &body) const
{
	Response	res;

	res.statusCode = statusCode;
	if (!contentType.empty())
		res.headers["Content-Type"] = contentType;
	res.body = body;
	return (res);
}

std::string	ResponseBuilder::generateAutoindex(const std::string &fsPath, const std::string &urlPath) const
{
	DIR		*dir;
	struct dirent	*entry;
	std::string	body;

	dir = opendir(fsPath.c_str());
	if (dir == NULL)
		return (buildDefaultErrorBody(403));
	body = "<html><body><h1>Index of ";
	body += fsPath;
	body += "</h1><ul>";
	while ((entry = readdir(dir)) != NULL)
	{
		body += "<li><a href=\"";
		body += joinPath(urlPath, entry->d_name);
		body += "\">";
		if (entry->d_type == DT_DIR)
			body += "&#128193; ";
		if (entry->d_type == DT_LNK)
			body += "&#128279; ";
		if (entry->d_type == DT_UNKNOWN)
			body += "&#8226; ";
		body += entry->d_name;
		body += "</a></li>";
	}
	body += "</ul></body></html>";
	closedir(dir);
	return (body);
}


// Utils UPLOAD
std::string	ResponseBuilder::joinPath(const std::string &dir, const std::string &file) const
{
	if (dir.empty())
		return (file);
	if (dir[dir.size() - 1] == '/')
		return (dir + file);
	return (dir + "/" + file);
}

std::string	ResponseBuilder::generateUploadFilename(void) const
{
	std::ostringstream	name;

	name << "upload_" << time(NULL) << ".dat";
	return (name.str());
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

bool	ResponseBuilder::isSafeFilename(const std::string &name) const
{
	if (name.find("/") != std::string::npos || name.find("..") != std::string::npos)
		return (false);
	return (true);
}

bool	ResponseBuilder::writeFile(const std::string &path,
	const std::string &content) const
{
	std::ofstream	file;

	file.open(path.c_str(), std::ios::out | std::ios::binary);
	if (!file.is_open())
		return (false);
	file.write(content.c_str(), content.size());
	if (!file.good())
		return (false);
	file.close();
	return (true);
}

std::string	ResponseBuilder::extractParentPath(const std::string &path) const
{
	size_t	pos;

	pos = path.rfind('/');
	if (pos == std::string::npos)
		return (".");
	if (pos == 0)
		return ("/");
	return (path.substr(0, pos));
}

bool	ResponseBuilder::isParentWritable(const std::string &path) const
{
	return (access(extractParentPath(path).c_str(), W_OK) == 0);
}
