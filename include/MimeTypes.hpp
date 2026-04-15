#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <string>
#include <map>

class	MimeTypes
{
	public:
		static std::string	getType(const std::string &path);

	private:
		static std::map<std::string, std::string>	_types;
		
		static std::map<std::string, std::string>	initTypes();
};

#endif
