#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include "Config.hpp"

class	ConfigParser
{
	public:
		ConfigParser(const std::string &path);

		std::vector<ServerConfig>	parse();

	private:
		std::string			_path;
		std::ifstream			_file;
		std::vector<ServerConfig>	_configs;

		void		parseServer(ServerConfig &config);
		void		parseLocation(Route &route);
		std::string	nextToken();
		void		expectToken(const std::string &token);
		void		parseErrorPage(ServerConfig &config);
		void		parseListen(ServerConfig &config);
};

#endif
