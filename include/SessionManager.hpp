#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include <string>
#include <map>

class SessionManager
{
	public:
		SessionManager();
		std::string	resolveSession(const std::string &cookieHeader, bool &isNew);
		int		incrementVisits(const std::string &sessionId);
	private:
		std::map<std::string, int>	_sessions;
		unsigned long			_counter;
		std::string			generateSessionId();
		std::string			extractCookieValue(const std::string &cookieHeader, const std::string &key);
};

#endif
