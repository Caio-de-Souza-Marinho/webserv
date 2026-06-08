#include "../../include/ResponseBuilder.hpp"
#include <cstddef>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <unistd.h>

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
