#include "../../include/ResponseBuilder.hpp"
#include "../../include/MimeTypes.hpp"
#include <fstream>
#include <sstream>

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

