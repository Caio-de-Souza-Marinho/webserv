#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <cstddef>
#include <string>
#include "Request.hpp"

class	RequestParser
{
	public:
		RequestParser();

		enum State
		{
			REQUEST_LINE,
			HEADERS,
			BODY,
			COMPLETE,
			PARSE_ERROR
		};

		State	parse(Request &request, std::string &buffer, size_t mazBodySize);
		void	reset();

	private:
		State	state;
		size_t	bodyBytesRead;
		size_t	expectedBodySize;

		size_t	currentChunkSize;
		bool	readChunkSize;

		bool		parseRequestLine(Request &request, std::string &buffer);
		bool		parseHeaders(Request &request, std::string &buffer);
		bool		parseBody(Request &request, std::string &buffer);
		std::string	extractLine(std::string &buffer);
		void		parseURI(Request &request);
		size_t		maxBodySize;
};

#endif
