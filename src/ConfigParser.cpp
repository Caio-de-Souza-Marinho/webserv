#include "../include/ConfigParser.hpp"
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace
{
	bool	isSpecialToken(char c)
	{
		return (c == '{' || c == '}' || c == ';');
	}

	std::runtime_error	configError(const std::string &path,
			const std::string &message)
	{
		return (std::runtime_error("Config error in " + path + ": " + message));
	}

	bool	isDigits(const std::string &value)
	{
		if (value.empty())
			return (false);
		for (size_t i = 0; i < value.size(); i++)
		{
			if (!std::isdigit(static_cast<unsigned char>(value[i])))
				return (false);
		}
		return (true);
	}

	long	parseLong(const std::string &path, const std::string &value,
			const std::string &field)
	{
		long			number;
		std::istringstream	stream(value);

		if (!isDigits(value) || !(stream >> number))
			throw configError(path, "invalid " + field + ": " + value);
		return (number);
	}

	size_t	parseBodySize(const std::string &path, const std::string &value)
	{
		std::string	numberPart;
		size_t		multiplier;
		unsigned long	number;
		std::istringstream	stream;

		if (value.empty())
			throw configError(path, "missing client_max_body_size value");
		multiplier = 1;
		numberPart = value;
		if (value[value.size() - 1] == 'm' || value[value.size() - 1] == 'M')
		{
			multiplier = 1024 * 1024;
			numberPart = value.substr(0, value.size() - 1);
		}
		if (!isDigits(numberPart))
			throw configError(path, "invalid client_max_body_size: " + value);
		stream.str(numberPart);
		if (!(stream >> number))
			throw configError(path, "invalid client_max_body_size: " + value);
		return (static_cast<size_t>(number * multiplier));
	}

	std::string	requiredValue(const std::string &path,
			const std::string &value, const std::string &field)
	{
		if (value.empty() || isSpecialToken(value[0]))
			throw configError(path, "missing value for " + field);
		return (value);
	}
}

ConfigParser::ConfigParser(const std::string &path) :
	_path(path),
	_file(path.c_str())
{
	if (!_file.is_open())
		throw configError(_path, "could not open file");
}

std::vector<ServerConfig>	ConfigParser::parse()
{
	std::string	token;

	_configs.clear();
	while (!(token = nextToken()).empty())
	{
		if (token != "server")
			throw configError(_path, "expected 'server', got '" + token + "'");

		ServerConfig	config;
		parseServer(config);
		_configs.push_back(config);
	}
	if (_configs.empty())
		throw configError(_path, "no server block found");
	return (_configs);
}

void	ConfigParser::parseServer(ServerConfig &config)
{
	std::string			token;
	std::string			serverRoot;
	std::vector<std::string>	serverIndex;
	bool				closed;

	expectToken("{");
	closed = false;
	while (!(token = nextToken()).empty())
	{
		if (token == "}")
		{
			closed = true;
			break ;
		}
		if (token == "listen")
			parseListen(config);
		else if (token == "root")
		{
			serverRoot = requiredValue(_path, nextToken(), "root");
			expectToken(";");
		}
		else if (token == "index")
		{
			serverIndex.clear();
			while (!(token = nextToken()).empty() && token != ";")
			{
				if (isSpecialToken(token[0]))
					throw configError(_path, "invalid index value: " + token);
				serverIndex.push_back(token);
			}
			if (token.empty())
				throw configError(_path, "missing ';' after index");
			if (serverIndex.empty())
				throw configError(_path, "index requires at least one file");
		}
		else if (token == "client_max_body_size")
		{
			config.maxBodySize = parseBodySize(_path,
					requiredValue(_path, nextToken(), "client_max_body_size"));
			expectToken(";");
		}
		else if (token == "error_page")
			parseErrorPage(config);
		else if (token == "location")
		{
			Route	route;

			route.root = serverRoot;
			route.index = serverIndex;
			route.path = requiredValue(_path, nextToken(), "location path");
			expectToken("{");
			parseLocation(route);
			config.routes.push_back(route);
		}
		else
			throw configError(_path, "unknown server directive: " + token);
	}
	if (!closed)
		throw configError(_path, "missing closing '}' for server block");
	for (size_t i = 0; i < config.routes.size(); i++)
	{
		if (config.routes[i].root.empty())
			config.routes[i].root = serverRoot;
		if (config.routes[i].index.empty())
			config.routes[i].index = serverIndex;
	}
}

void	ConfigParser::parseLocation(Route &route)
{
	std::string	token;
	bool		closed;

	closed = false;
	while (!(token = nextToken()).empty())
	{
		if (token == "}")
		{
			closed = true;
			break ;
		}
		if (token == "allow_methods")
		{
			route.methods.clear();
			while (!(token = nextToken()).empty() && token != ";")
			{
				if (isSpecialToken(token[0]))
					throw configError(_path, "invalid method: " + token);
				route.methods.insert(token);
			}
			if (token.empty())
				throw configError(_path, "missing ';' after allow_methods");
			if (route.methods.empty())
				throw configError(_path, "allow_methods requires at least one method");
		}
		else if (token == "autoindex")
		{
			token = requiredValue(_path, nextToken(), "autoindex");
			if (token == "on")
				route.autoindex = true;
			else if (token == "off")
				route.autoindex = false;
			else
				throw configError(_path, "autoindex must be 'on' or 'off'");
			expectToken(";");
		}
		else if (token == "root")
		{
			route.root = requiredValue(_path, nextToken(), "root");
			expectToken(";");
		}
		else if (token == "index")
		{
			route.index.clear();
			while (!(token = nextToken()).empty() && token != ";")
			{
				if (isSpecialToken(token[0]))
					throw configError(_path, "invalid index value: " + token);
				route.index.push_back(token);
			}
			if (token.empty())
				throw configError(_path, "missing ';' after index");
			if (route.index.empty())
				throw configError(_path, "index requires at least one file");
		}
		else if (token == "upload_path")
		{
			route.uploadPath = requiredValue(_path, nextToken(), "upload_path");
			expectToken(";");
		}
		else if (token == "return")
		{
			route.redirectCode = static_cast<int>(parseLong(_path,
						requiredValue(_path, nextToken(), "return code"),
						"return code"));
			if (route.redirectCode < 100 || route.redirectCode > 599)
				throw configError(_path, "return code must be between 100 and 599");
			route.redirectUrl = requiredValue(_path, nextToken(), "return URL");
			expectToken(";");
		}
		else if (token == "cgi")
		{
			std::string	extension;
			std::string	interpreter;

			extension = requiredValue(_path, nextToken(), "cgi extension");
			interpreter = requiredValue(_path, nextToken(), "cgi interpreter");
			route.cgiHandlers[extension] = interpreter;
			expectToken(";");
		}
		else
			throw configError(_path, "unknown location directive: " + token);
	}
	if (!closed)
		throw configError(_path, "missing closing '}' for location block");
}

std::string	ConfigParser::nextToken()
{
	char		c;
	std::string	token;

	while (_file.get(c))
	{
		if (std::isspace(static_cast<unsigned char>(c)))
			continue ;
		if (c == '#')
		{
			while (_file.get(c) && c != '\n')
				;
			continue ;
		}
		if (isSpecialToken(c))
			return (std::string(1, c));
		token += c;
		while (_file.get(c))
		{
			if (std::isspace(static_cast<unsigned char>(c)))
				break ;
			if (c == '#')
			{
				while (_file.get(c) && c != '\n')
					;
				break ;
			}
			if (isSpecialToken(c))
			{
				_file.unget();
				break ;
			}
			token += c;
		}
		return (token);
	}
	return ("");
}

void	ConfigParser::expectToken(const std::string &expected)
{
	std::string	token;

	token = nextToken();
	if (token != expected)
	{
		if (token.empty())
			throw configError(_path, "expected '" + expected + "', got end of file");
		throw configError(_path, "expected '" + expected + "', got '" + token + "'");
	}
}

void	ConfigParser::parseErrorPage(ServerConfig &config)
{
	std::string	codeToken;
	std::string	path;
	int		code;

	codeToken = requiredValue(_path, nextToken(), "error_page code");
	code = static_cast<int>(parseLong(_path, codeToken, "error_page code"));
	if (code < 100 || code > 599)
		throw configError(_path, "error_page code must be between 100 and 599");
	path = requiredValue(_path, nextToken(), "error_page path");
	config.errorPages[code] = path;
	expectToken(";");
}

void	ConfigParser::parseListen(ServerConfig &config)
{
	std::string	value;
	std::string	portPart;
	size_t		colon;
	long		port;

	value = requiredValue(_path, nextToken(), "listen");
	colon = value.find(':');
	if (colon != std::string::npos)
	{
		if (colon == 0 || colon == value.size() - 1)
			throw configError(_path, "listen must be host:port or port");
		config.host = value.substr(0, colon);
		portPart = value.substr(colon + 1);
	}
	else
	{
		config.host = "0.0.0.0";
		portPart = value;
	}
	port = parseLong(_path, portPart, "listen port");
	if (port < 1 || port > 65535)
		throw configError(_path, "listen port must be between 1 and 65535");
	config.port = static_cast<int>(port);
	expectToken(";");
}
