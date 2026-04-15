#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <string>
#include "Request.hpp"

class	RequestParser
{
	public:
		enum State
		{
			REQUEST_LINE,
			HEADERS,
			BODY,
			COMPLETE
		};

	private:
		State	state;
		size_t	bodyBytesRead;
		size_t	expectedBodySize;

		// chunked
		size_t	currentChunkSize;
		bool	readChunkSize;

		bool		parseRequestLine(Request &req, std::string &buffer);
		bool		parseHeaders(Request &req, std::string &buffer);
		bool		parseBody(Request &req, std::string &buffer);

		std::string	extractLine(std::string &buffer);
		void		parseURI(Request &req);

};

#endif
