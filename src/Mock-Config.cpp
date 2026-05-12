/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Mock-Config.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marcudos <marcudos@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 14:52:00 by marcudos          #+#    #+#             */
/*   Updated: 2026/05/12 14:55:12 by marcudos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Config.hpp"

ServerConfig makeMockConfig()
{
	ServerConfig config;

	Route root;
	root.path = "/";
	root.root = "./www";
	root.index.push_back("index.html");
	root.methods.insert("GET");
	root.autoindex = false;

	Route uploads;
	uploads.path = "/uploads";
	uploads.root = "./www/uploads";
	uploads.index.push_back("index.html");
	uploads.methods.insert("GET");
	uploads.methods.insert("POST");
	uploads.methods.insert("DELETE");
	uploads.autoindex = true;
	uploads.uploadPath = "./www/uploads";

	Route redirect;
	redirect.path = "/old";
	redirect.redirectCode = 301;
	redirect.redirectUrl = "/new";
	redirect.methods.insert("GET");

	config.routes.push_back(root);
	config.routes.push_back(uploads);
	config.routes.push_back(redirect);
	config.errorPages[404] = "./www/errors/404.html";

	return config;
}
