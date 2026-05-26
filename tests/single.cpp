#include "../include/ConfigParser.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Router.hpp"
#include "../include/ResponseBuilder.hpp"
#include "../include/Response.hpp"
#include <iostream>

int	main(void)
{
	std::string raw =
		"POST /uploads HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Disposition: attachment; filename=\"test-upload.txt\"\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello world";

	ConfigParser parser("config/default.conf");
	std::vector<ServerConfig> servers = parser.parse();

	if (servers.empty())
	{
		std::cerr << "No servers\n";
		return (1);
	}

	Request req;
	RequestParser reqParser;
	reqParser.parse(req, raw);

	const ServerConfig &config = servers[0];

	Router router;
	const Route *route = router.matchRoute(req, config);

	ResponseBuilder builder;
	Response res = builder.buildResponse(req, route, config);

	std::cout << "REQUEST: " << req.method << " " << req.path << "\n";

	if (route)
	{
		std::cout << "ROUTE: " << route->path << "\n";
		std::cout << "ROOT: " << route->root << "\n";
		std::cout << "RESOLVED: " << router.resolvePath(*route, req) << "\n";
	}
	else
		std::cout << "ROUTE: NULL\n";

	std::cout << "STATUS: " << res.statusCode << " "
		<< Response::getStatusMessage(res.statusCode) << "\n";

	std::cout << "\nRAW RESPONSE:\n";
	std::cout << res.build() << "\n";

	return (0);
}
