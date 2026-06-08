#include "../../include/ResponseBuilder.hpp"
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
