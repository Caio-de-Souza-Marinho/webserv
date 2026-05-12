/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router-Tester.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 17:51:47 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 19:16:16 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServer.hpp"
#include "../include/Request.hpp"
#include "../include/RequestParser.hpp"
#include "../include/Config.hpp"
#include "../include/Response.hpp"
#include "../include/Router.hpp"
#include "../include/ResponseBuilder.hpp"
#include "../include/Colors.hpp"

#include <iostream>

// -- Tester Resolve Path
void	printResolvePath(const std::string &path)
{
	ServerConfig	config = makeMockConfig();
	Request		req;
	Router		router;
	const Route	*route;

	req.path = path;
	route = router.matchRoute(req, config);

	std::cout << "request: " << path << "\n";
	if (route)
	{
		std::cout << "route:   " << route->path << "\n";
		std::cout << "root:    " << route->root << "\n";
		std::cout << "file:    " << router.resolvePath(*route, req) << "\n";
	}
	else
		std::cout << "route:   NULL\n";
	std::cout << "---\n";
}

void	testResolvePath(void)
{
	printResolvePath("/index.html");
	printResolvePath("/uploads");
	printResolvePath("/uploads/test.txt");
	printResolvePath("/uploads/test/");
	printResolvePath("/upload");
}

// -- Tester Router match
void	printRouteMatch(const std::string &path)
{
	ServerConfig	config = makeMockConfig();
	Request		req;
	Router		router;
	const Route	*route;

	req.path = path;
	route = router.matchRoute(req, config);

	std::cout << "\nrequest path: " << path << "\n";
	if (route)
	{
		std::cout << "matched route: " << route->path << "\n";
		std::cout << "route root:    " << route->root << "\n";
		std::cout << "resolved path: "
			<< router.resolvePath(*route, req) << "\n";
	}
	else
		std::cout << "matched route: NULL\n";
	std::cout << "---\n";
}

void	testRouterMatch(void)
{
	std::cout << "\n========== Router matchRoute ==========\n";

	printRouteMatch("/index.html");
	printRouteMatch("/about.html");
	printRouteMatch("/uploads");
	printRouteMatch("/uploads/test.txt");
	printRouteMatch("/uploads/test/");
	printRouteMatch("/upload");
	printRouteMatch("/uploadsABC");
	printRouteMatch("/old");
	printRouteMatch("/not-found.html");
}

// -- Tester Router methods
void	testRouterMethods(void)
{
	ServerConfig	config = makeMockConfig();
	Router		router;
	const Route	*route;
	Request		req;

	std::cout << "\n========== Router isMethodAllowed ==========\n";

	req.path = "/uploads/test.txt";
	route = router.matchRoute(req, config);

	if (!route)
	{
		std::cout << "No route found\n";
		return ;
	}

	std::cout << "route: " << route->path << "\n";
	std::cout << "GET:    " << router.isMethodAllowed(*route, "GET") << "\n";
	std::cout << "POST:   " << router.isMethodAllowed(*route, "POST") << "\n";
	std::cout << "DELETE: " << router.isMethodAllowed(*route, "DELETE") << "\n";
	std::cout << "PUT:    " << router.isMethodAllowed(*route, "PUT") << "\n";
}

void	testBuildResponse(const std::string &method, const std::string &path)
{
	ServerConfig		config = makeMockConfig();
	Request			req;
	Router			router;
	ResponseBuilder	builder;
	const Route		*route;
	Response		res;

	req.method = method;
	req.path = path;
	req.version = "HTTP/1.1";
	req.isComplete = true;

	route = router.matchRoute(req, config);
	res = builder.buildResponse(req, route, config);

	std::cout << "\n========== " << method << " " << path << " ==========\n";
	if (route)
		std::cout << "matched route: " << route->path << "\n";
	else
		std::cout << "matched route: NULL\n";
	std::cout << res.build() << "\n";
}

void makeTesteBuildResponse(void)
{
	testBuildResponse("GET", "/index.html");
	testBuildResponse("GET", "/not-found.html");
	testBuildResponse("GET", "/uploads");
	testBuildResponse("GET", "/old");
	testBuildResponse("POST", "/uploads");
	testBuildResponse("DELETE", "/uploads/test.txt");
}
