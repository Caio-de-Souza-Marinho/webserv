#include "../include/Logger.hpp"
#include <ctime>

Logger::Level Logger::_level = Logger::DEBUG;

std::string	Logger::levelToString(Level level)
{
	switch (level)
	{
		case DEBUG:
			return ("DEBUG");
		case INFO:
			return ("INFO");
		case WARNING:
			return ("WARNING");
		case ERROR:
			return ("ERROR");
		default:
			return ("UNKNOWN");
	}
}

void	Logger::log(Level level, const std::string &message)
{
	if (level < _level)
		return;

	time_t      now = time(NULL);
	struct tm   *tm_info = localtime(&now);
	char        timebuf[20];

	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

	std::cerr	<< "[" << timebuf << "] "
			<< "[" << levelToString(level) << "] "
			<< message << std::endl;
}

void	Logger::debug(const std::string &message)
{
	log(DEBUG, message);
}

void	Logger::info(const std::string &message)
{
	log(INFO, message);
}

void	Logger::warning(const std::string &message)
{
	log(WARNING, message);
}

void	Logger::error(const std::string &message)
{
	log(ERROR, message);
}

void	Logger::setLevel(Level level)
{
	_level = level;
}
