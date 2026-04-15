#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include "Request.hpp"
#include "Config.hpp"

class Client;

class	CGIHandler
{
	public:
		CGIHandler();

		void	execute(Client &client, const std::string &scriptPath, const std::string &interpreter);

	private:
		std::vector<std::string>	buildEnv(const Request &request, const std::string &scriptPath, const Route &route) const;
		char**				buildEnvp(const std::vector<std::string> &env) const;
		void				freeEnvp(char **envp) const;
		char**				buildArgv(const std::string &interpreter, const std::string &scriptPath) const;
		void				freeArgv(char **argv) const;
};

#endif
