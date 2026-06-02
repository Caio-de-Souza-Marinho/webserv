#include "../include/Request.hpp"

Request::Request() :
	contentLength(0),
	isChunked(false),
	isComplete(false),
	keepAlive(false),
	errorCode(0)
{}

void	Request::reset()
{
		method.clear();
		rawUri.clear();
		uri.clear();
		path.clear();
		host.clear();
		query.clear();
		version.clear();
		headers.clear();
		body.clear();
		contentLength = 0;
		isChunked = false;
		isComplete = false;
		keepAlive = false;
		errorCode = 0;
}
