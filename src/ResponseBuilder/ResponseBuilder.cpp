/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 18:09:16 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 19:04:56 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/ResponseBuilder.hpp"

ResponseBuilder::ResponseBuilder() {};

Response ResponseBuilder::buildResponse(const Request &request, 
		const Route *route, const ServerConfig &config)
{
	if (route == NULL)
		return (handleError(404, config));

	if (route->redirectCode != 0)
		return (handleRedirect(*route));

	if (route->methods.find(request.method) == route->methods.end())
		return (handleError(405, config));

	if (request.method == "GET")
		return (handleGET(request, *route));

	if (request.method == "POST")
		return (handlePOST(request, *route));

	if (request.method == "DELETE")
		return (handleDELETE(request, *route));

	return (handleError(501, config));
}


Response ResponseBuilder::buildErrorResponse(int code, const ServerConfig &config)
{
	return (handleError(code, config));
}

// handles


