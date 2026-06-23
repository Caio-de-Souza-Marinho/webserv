#include "../../include/MultipartParser.hpp"
#include <cstddef>

MultipartParser::MultipartParser() {}

bool	MultipartParser::isMultipart(const std::string &contentType) const
{
	return (contentType.find("multipart/form-data") != std::string::npos);
}

std::string	MultipartParser::extractBoundary(const std::string &contentType) const
{
	size_t	pos = contentType.find("boundary=");
	if (pos == std::string::npos)
		return ("");
	pos += 9; 

	std::string	boundary = contentType.substr(pos);

	if (!boundary.empty() && boundary[0] == '"')
	{
		boundary.erase(0, 1);
		size_t	endQuote = boundary.find('"');
		if (endQuote != std::string::npos)
			boundary = boundary.substr(0, endQuote);
	}
	return (boundary);
}

std::vector<std::string>	MultipartParser::splitParts(const std::string &body, const std::string &boundary) const
{
	std::vector<std::string>	parts;
	std::string			delimiter = "--" + boundary;
	size_t				start;
	size_t				next;

	start = body.find(delimiter);
	if (start == std::string::npos)
		return (parts);
	start += delimiter.size();

	while (start < body.size())
	{
		if (body.compare(start, 2, "--") == 0)
			break ;

		if (body.compare(start, 2, "\r\n") == 0)
			start += 2;

		next = body.find(delimiter, start);
		if (next == std::string::npos)
			break ;

		size_t	len = next - start;
		if (len >= 2 && body.compare(next - 2, 2, "\r\n") == 0)
			len -= 2;

		parts.push_back(body.substr(start, len));
		start = next + delimiter.size();
	}
	return (parts);
}

MultipartPart	MultipartParser::parseOnePart(const std::string &rawPart) const
{
	MultipartPart	part;

	size_t	headerEnd = rawPart.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return (part);

	std::string	headers = rawPart.substr(0, headerEnd);
	part.content = rawPart.substr(headerEnd + 4); 

	size_t	fn = headers.find("filename=\"");
	if (fn == std::string::npos)
		return (part); 

	fn += 10; 
	size_t	endQuote = headers.find('"', fn);
	if (endQuote == std::string::npos)
		return (part);

	part.filename = headers.substr(fn, endQuote - fn);
	return (part);
}

std::vector<MultipartPart>	MultipartParser::parse(const std::string &contentType, const std::string &body) const
{
	std::vector<MultipartPart>	files;

	std::string	boundary = extractBoundary(contentType);
	if (boundary.empty())
		return (files);

	std::vector<std::string>	rawParts = splitParts(body, boundary);
	for (size_t i = 0; i < rawParts.size(); i++)
	{
		MultipartPart	part = parseOnePart(rawParts[i]);
		if (!part.filename.empty())
			files.push_back(part);
	}
	return (files);
}
