#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Logger.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <cstring>
#include <cstdlib>

CGIHandler::CGIHandler() {}

void	CGIHandler::setNonBlocking(int fd) const
{
	int	flags = fcntl(fd, F_GETFL, 0);
	if (flags != -1)
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Builds the CGI/1.1 environment as a vector of "KEY=VALUE" strings.
std::vector<std::string>	CGIHandler::buildEnv(const Request &request,
	const std::string &scriptPath, const ServerConfig &config) const
{
	std::vector<std::string>	env;
	std::ostringstream		oss;

	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REDIRECT_STATUS=200");	// required by php-cgi
	env.push_back("REQUEST_METHOD=" + request.method);
	env.push_back("QUERY_STRING=" + request.query);
	env.push_back("SCRIPT_FILENAME=" + scriptPath);
	env.push_back("SCRIPT_NAME=" + request.path);
	env.push_back("PATH_INFO=" + request.path);
	env.push_back("SERVER_NAME=" + config.host);
	env.push_back("REQUEST_URI=" + request.rawUri);

	oss << config.port;
	env.push_back("SERVER_PORT=" + oss.str());

	// Body length: CGI scripts read CONTENT_LENGTH bytes from stdin.
	oss.str("");
	oss << request.body.size();
	env.push_back("CONTENT_LENGTH=" + oss.str());

	// Content-Type comes from the request header (if any).
	std::map<std::string, std::string>::const_iterator	ct = request.headers.find("content-type");
	if (ct != request.headers.end())
		env.push_back("CONTENT_TYPE=" + ct->second);

	// Every request header is also exported as HTTP_<HEADER> (upper, '-' -> '_').
	std::map<std::string, std::string>::const_iterator	it;
	for (it = request.headers.begin(); it != request.headers.end(); ++it)
	{
		std::string	key = "HTTP_";
		for (size_t i = 0; i < it->first.size(); i++)
		{
			char	c = it->first[i];
			if (c == '-')
				c = '_';
			else
				c = static_cast<char>(::toupper(c));
			key += c;
		}
		env.push_back(key + "=" + it->second);
	}

	return (env);
}

// Converts the vector<string> env into the char** that execve() expects.
char**	CGIHandler::buildEnvp(const std::vector<std::string> &env) const
{
	char	**envp = new char*[env.size() + 1];

	for (size_t i = 0; i < env.size(); i++)
	{
		envp[i] = new char[env[i].size() + 1];
		std::strcpy(envp[i], env[i].c_str());
	}
	envp[env.size()] = NULL;
	return (envp);
}

void	CGIHandler::freeEnvp(char **envp) const
{
	if (!envp)
		return ;
	for (size_t i = 0; envp[i]; i++)
		delete [] envp[i];
	delete [] envp;
}

// argv = { interpreter, scriptPath, NULL }  (e.g. {"/usr/bin/python3", "./www/cgi-bin/test.py", NULL})
char**	CGIHandler::buildArgv(const std::string &interpreter, const std::string &scriptPath) const
{
	char	**argv = new char*[3];

	argv[0] = new char[interpreter.size() + 1];
	std::strcpy(argv[0], interpreter.c_str());
	argv[1] = new char[scriptPath.size() + 1];
	std::strcpy(argv[1], scriptPath.c_str());
	argv[2] = NULL;
	return (argv);
}

void	CGIHandler::freeArgv(char **argv) const
{
	if (!argv)
		return ;
	for (size_t i = 0; argv[i]; i++)
		delete [] argv[i];
	delete [] argv;
}

bool	CGIHandler::execute(Client &client, const std::string &scriptPath, const std::string &interpreter)
{
	int	pipeIn[2];	// parent writes request body  -> child stdin
	int	pipeOut[2];	// child writes its output     -> parent reads

	// Resolve o script para path absoluto: o filho faz chdir() para o diretório
	// do script, então um path relativo não funcionaria mais após isso.
	std::string	absPath = scriptPath;
	if (!scriptPath.empty() && scriptPath[0] != '/')
	{
		char	cwd[4096];
		if (getcwd(cwd, sizeof(cwd)))
			absPath = std::string(cwd) + "/" + scriptPath;
	}

	// CORREÇÃO: resolve o interpreter para path absoluto ANTES do fork.
	// O filho faz chdir() para o diretório do script, então um path relativo
	// como "./cgi_tester" deixa de existir após o chdir. Resolvendo aqui,
	// no processo pai, o cwd ainda é o diretório raiz do servidor.
	std::string	absInterpreter = interpreter;
	if (!interpreter.empty() && interpreter[0] != '/')
	{
		char	cwd[4096];
		if (getcwd(cwd, sizeof(cwd)))
			absInterpreter = std::string(cwd) + "/" + interpreter;
	}

	if (pipe(pipeIn) == -1)
	{
		Logger::error("CGI: pipe(in) failed");
		return (false);
	}
	if (pipe(pipeOut) == -1)
	{
		Logger::error("CGI: pipe(out) failed");
		close(pipeIn[0]);
		close(pipeIn[1]);
		return (false);
	}

	char	**envp = buildEnvp(buildEnv(client.request, absPath, client.server->config));
	char	**argv = buildArgv(absInterpreter, absPath); // usa absInterpreter

	pid_t	pid = fork();
	if (pid == -1)
	{
		Logger::error("CGI: fork failed");
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);
		freeEnvp(envp);
		freeArgv(argv);
		return (false);
	}

	if (pid == 0)
	{
		// --- child ---
		dup2(pipeIn[0], STDIN_FILENO);		// stdin  reads from pipeIn
		dup2(pipeOut[1], STDOUT_FILENO);	// stdout writes to pipeOut

		// the child does not need any of the raw pipe ends anymore
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);

		// run the script from its own directory (scripts often expect this)
		std::string::size_type	slash = absPath.find_last_of('/');
		if (slash != std::string::npos)
			if (chdir(absPath.substr(0, slash).c_str()) != 0)
				_exit(1);

		execve(argv[0], argv, envp);
		// only reached if execve failed
		_exit(1);
	}

	// --- parent ---
	close(pipeIn[0]);	// parent does not read the child's stdin pipe
	close(pipeOut[1]);	// parent does not write the child's stdout pipe

	freeEnvp(envp);		// the child got its own copy via fork
	freeArgv(argv);

	client.cgiPid	   = pid;
	client.cgiInputFd  = pipeIn[1];		// we write the body here
	client.cgiOutputFd = pipeOut[0];	// we read the result here
	client.cgiBuffer.clear();
	client.cgiDone     = false;

	setNonBlocking(client.cgiInputFd);
	setNonBlocking(client.cgiOutputFd);

	client.state = CGI_RUNNING;
	return (true);
}
