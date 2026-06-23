#include "../include/SessionManager.hpp"
#include <cstddef>
#include <iterator>
#include <sstream>

SessionManager::SessionManager() : _counter(0) {}

std::string	SessionManager::generateSessionId()
{
	std::ostringstream	id;

	_counter++;
	id << "sess_" << time(NULL) << "_" << _counter;
	return (id.str());
}

std::string	SessionManager::extractCookieValue(const std::string &cookieHeader, const std::string &key)
{
	std::string	target = key + "=";
	size_t		pos = cookieHeader.find(target);

	if (pos == std::string::npos)
		return ("");
	pos += target.size();

	size_t	end = cookieHeader.find(';', pos);
	if (end == std::string::npos)
		end = cookieHeader.size();
	
	std::string value = cookieHeader.substr(pos, end - pos);
	
	size_t start = value.find_first_not_of(" \t");
	if (start == std::string::npos)
		return ("");
	value = value.substr(start);
	return (value);
}

std::string	SessionManager::resolveSession(const std::string &cookieHeader, bool &isNew)
{
	std::string	id = extractCookieValue(cookieHeader, "session_id");

	if (!id.empty() && _sessions.find(id) != _sessions.end())
	{
		isNew = false;
		return (id);
	}

	id = generateSessionId();
	_sessions[id] = 0;
	isNew = true;
	return (id);
}

int	SessionManager::incrementVisits(const std::string &sessionId)
{
	_sessions[sessionId]++;
	return (_sessions[sessionId]);
}
