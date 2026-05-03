#include "../include/RequestParser.hpp"
#include "../include/Logger.hpp"
#include <sstream>
#include <algorithm>

RequestParser::RequestParser() :
	state(REQUEST_LINE),
	bodyBytesRead(0),
	expectedBodySize(0),
	currentChunkSize(0),
	readChunkSize(true)
{}

void	RequestParser::reset()
{
	state = REQUEST_LINE;
	bodyBytesRead = 0;
	expectedBodySize = 0;
	currentChunkSize = 0;
	readChunkSize = true;
}

RequestParser::State	RequestParser::parse(Request &request, std::string &buffer)
{
	while (state != COMPLETE)
	{
		if (state == REQUEST_LINE)
		{
			if (!parseRequestLine(request, buffer))
				return (state);
		}
		else if (state == HEADERS)
		{
			if (!parseHeaders(request, buffer))
				return (state);
		}
		else if (state == BODY)
		{
			if (!parseBody(request, buffer))
				return (state);
		}
	}
	return (state);
}

bool	RequestParser::parseRequestLine(Request &request, std::string &buffer)
{
	std::string	line = extractLine(buffer);
	if (line.empty())
		return (false); // need more data
	
	std::istringstream	ss(line);
	if (!(ss >> request.method >> request.rawUri >> request.version))
	{
		Logger::warning("malformed request line: " + line);
		return (false);
	}

	// validate method - uppsercase letters only
	for (size_t i = 0; i < request.method.size(); i++)
	{
		if (!isupper(request.method[i]))
		{
			Logger::warning("invalid method: " + request.method);
			return (false);
		}
	}

	// validate version
	if (request.version != "HTTP/1.0" && request.version != "HTTP/1.1")
	{
		Logger::warning("unsupported HTTP versoin: " + request.version);
		return (false);
	}

	parseURI(request);
	state = HEADERS;

	return (true);
}

std::string	RequestParser::extractLine(std::string &buffer)
{
	size_t	pos = buffer.find("\r\n");
	if (pos == std::string::npos)
		return ("");

	std::string	line = buffer.substr(0, pos);
	buffer.erase(0, pos + 2);

	return (line);
}

void	RequestParser::parseURI(Request &request)
{
	size_t	q = request.rawUri.find('?');
	if (q != std::string::npos)
	{
		request.path = request.rawUri.substr(0, q);
		request.query = request.rawUri.substr(q + 1);
	}
	else
	{
		request.path = request.rawUri;
		request.query = "";
	}
	request.uri = request.path;

	// percent decode path
	std::string	decoded;
	for (size_t i = 0; i < request.path.size(); i++)
	{
		if (request.path[i] == '%' && i + 2 < request.path.size())
		{
			std::string	hex = request.path.substr(i + 1, 2);
			char		c = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
			decoded += c;
			i += 2;
		}
		else
			decoded += request.path[i];
	}
	request.path = decoded;
}

bool	RequestParser::parseHeaders(Request &request, std::string &buffer)
{
	while (true)
	{
		// check for \r\n before extracting
		if (buffer.find("\r\n") == std::string::npos)
			return (false);	// need more data - no \r\n found at all
		
		std::string	line = extractLine(buffer);
		
		// blank line = end of headers
		if (line.empty())
		{
			// extract key fields from headers
			std::map<std::string, std::string>::const_iterator	it;

			it = request.headers.find("host");
			if (it != request.headers.end())
				request.host = it->second;

			it = request.headers.find("content-length");
			if (it != request.headers.end())
			{
				std::istringstream	ss(it->second);
				ss >> request.contentLength;
				expectedBodySize = request.contentLength;
			}

			it = request.headers.find("transfer-encoding");
			if (it != request.headers.end() && it->second == "chunked")
				request.isChunked = true;

			it = request.headers.find("connection");
			if (it != request.headers.end())
			{
				std::string	val = it->second;
				// lowercase for comparison
				for (size_t i = 0; i < val.size(); i++)
					val[i] = tolower(val[i]);
				request.keepAlive = (val == "keep-alive");
			}
			else
				request.keepAlive = (request.version == "HTTP/1.1");

			// no body expected
			if (!request.isChunked && expectedBodySize == 0)
			{
				request.isComplete = true;
				state = COMPLETE;
			}
			else
				state = BODY;

			return (true);
		}

		// split on ": "
		size_t	colon = line.find(": ");
		if (colon == std::string::npos)
		{
			Logger::warning("malformed header: " + line);
			continue ;
		}

		std::string	key = line.substr(0, colon);
		std::string	val = line.substr(colon + 2);

		// lowercase key
		for (size_t i = 0; i < key.size(); i++)
			key[i] = tolower(key[i]);

		request.headers[key] = val;
	}
}

bool	RequestParser::parseBody(Request &request, std::string &buffer)
{
	if (!request.isChunked)
	{
		// regular body - read up to expectedBodySize
		size_t	available = buffer.size();
		size_t	needed = expectedBodySize - bodyBytesRead;
		size_t	toRead = std::min(available, needed);

		request.body.append(buffer, 0, toRead);
		buffer.erase(0, toRead);
		bodyBytesRead += toRead;

		if (bodyBytesRead >= expectedBodySize)
		{
			request.isComplete = true;
			state = COMPLETE;
		}

		return (true);
	}

	// chunked body
	while (true)
	{
		if (readChunkSize)
		{
			std::string	line = extractLine(buffer);
			if (line.empty() && buffer.find("\r\n") == std::string::npos)
				return (false);	// need more data

			// parse hex chunk size
			currentChunkSize = std::strtoul(line.c_str(), NULL, 16);
			readChunkSize = false;

			if (currentChunkSize == 0)
			{
				// last chunk - consume trailing \r\n
				extractLine(buffer);
				request.isComplete = true;
				state = COMPLETE;
				return (true);
			}
		}

		// read currentChunkSize bytes
		if (buffer.size() < currentChunkSize + 2)	// +2 for trailing \r\n
			return (false);	// read more data
		
		request.body.append(buffer, 0, currentChunkSize);
		buffer.erase(0, currentChunkSize);

		// consume trailing \r\n after chunk data
		extractLine(buffer);

		readChunkSize = true;
	}
}
