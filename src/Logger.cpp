#include "../include/Logger.hpp"

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

std::string	Logger::levelToColor(Level level)
{
	switch (level)
	{
		case DEBUG:
			return (BOLD_CYAN);
		case INFO:
			return (BOLD_GREEN);
		case WARNING:
			return (BOLD_YELLOW);
		case ERROR:
			return (BOLD_RED);
		default:
			return (RESET);
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

	if (USE_COLOR)
	{
		std::cerr	<< "[" << timebuf << "] "
				<< levelToColor(level)
				<< "[" << levelToString(level) << "] "
				<< RESET
				<< message << std::endl;
	}
	else
	{
		std::cerr	<< "[" << timebuf << "] "
				<< "[" << levelToString(level) << "] "
				<< message << std::endl;
	}
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
