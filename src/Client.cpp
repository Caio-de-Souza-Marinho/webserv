#include "../include/Client.hpp"

Client::Client() :
	fd(-1),
	readBuffer(),
	writeBuffer(),
	writeOffset(0),
	request(),
	response(),
	state(READING),
	requestComplete(false),
	lastActivity(0),
	server(NULL),
	parser(NULL),
	cgiPid(-1),
	cgiInputFd(-1),
	cgiOutputFd(-1),
	cgiBuffer(),
	cgiDone(false),
	ip(),
	port(0)
{}
