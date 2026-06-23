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

		// Forks the CGI process and wires its stdin/stdout to two pipes.
		// Stores cgiPid / cgiInputFd / cgiOutputFd on the client and sets
		// client.state = CGI_RUNNING. Returns false if a pipe/fork failed
		// (the caller should then answer 500).
		bool	execute(Client &client, const std::string &scriptPath, const std::string &interpreter,
				const std::string &scriptName, const std::string &pathInfo);

	private:
		std::vector<std::string>	buildEnv(const Request &request, const std::string &scriptPath, const ServerConfig &config,
							const std::string &scriptName, const std::string &pathInfo) const;
		char**				buildEnvp(const std::vector<std::string> &env) const;
		void				freeEnvp(char **envp) const;
		char**				buildArgv(const std::string &interpreter, const std::string &scriptPath) const;
		void				freeArgv(char **argv) const;
		void				setNonBlocking(int fd) const;
};

#endif
