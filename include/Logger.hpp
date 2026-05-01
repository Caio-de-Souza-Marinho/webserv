#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include "../include/Colors.hpp"
#include <iostream>
#include <ctime>

class	Logger
{
	public:
		enum Level
		{
			DEBUG,
			INFO,
			WARNING,
			ERROR
		};

		static void	log(Level level, const std::string &message);
		static void	debug(const std::string &message);
		static void	info(const std::string &message);
		static void	warning(const std::string &message);
		static void	error(const std::string &message);
		static void	setLevel(Level level);

	private:
		static Level	_level;

		static std::string	levelToString(Level level);
		static std::string	levelToColor(Level level);
};

#endif
