#ifndef MULTIPARTPARSER_HPP
#define MULTIPARTPARSER_HPP

#include <string>
#include <vector>

struct	MultipartPart
{
	std::string	filename;
	std::string	content;
};

class	MultipartParser
{
	public:
		MultipartParser();
		std::vector<MultipartPart>	parse(const std::string &contentType, const std::string &body) const;
		bool				isMultipart(const std::string &contentType) const;
	private:
		std::string			extractBoundary(const std::string &contentType) const;
		std::vector<std::string>	splitParts(const std::string &body, const std::string &boundery) const;
		MultipartPart			parseOnePart(const std::string &rayPart) const;
};

#endif
