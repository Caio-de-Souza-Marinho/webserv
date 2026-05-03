#include "../include/MimeTypes.hpp"

std::map<std::string, std::string> MimeTypes::_types = MimeTypes::initTypes();

std::map<std::string, std::string> MimeTypes::initTypes()
{
	std::map<std::string, std::string>	types;

	// text
	types[".html"]	= "text/html";
	types[".htm"]	= "text/html";
	types[".css"]	= "text/css";
	types[".js"]	= "application/javascript";
	types[".json"]	= "application/json";
	types[".xml"]	= "application/xml";
	types[".txt"]	= "text/plain";
	types[".csv"]	= "text/csv";

	// images
	types[".png"]	= "image/png";
	types[".jpg"]	= "image/jpeg";
	types[".jpeg"]	= "image/jpeg";
	types[".gif"]	= "image/gif";
	types[".ico"]	= "image/x-icon";
	types[".svg"]	= "image/svg+xml";
	types[".webp"]	= "image/webp";
	types[".bmp"]	= "image/bmp";

	// video/audio
	types[".mp4"]   = "video/mp4";
	types[".webm"]  = "video/webm";
	types[".mp3"]   = "audio/mpeg";
	types[".wav"]   = "audio/wav";
	types[".ogg"]   = "audio/ogg";

	// fonts
	types[".ttf"]   = "font/ttf";
	types[".woff"]  = "font/woff";
	types[".woff2"] = "font/woff2";

	// documents
	types[".pdf"]   = "application/pdf";
	types[".zip"]   = "application/zip";
	types[".tar"]   = "application/x-tar";
	types[".gz"]    = "application/gzip";

	return (types);
}

std::string	MimeTypes::getType(const std::string &path)
{
	size_t	dot = path.rfind('.');
	if (dot == std::string::npos)
		return ("application/octet-stream");

	std::string	ext = path.substr(dot);

	// lowercase the extension
	for (size_t i = 0; i < ext.size(); i++)
		ext[i] = tolower(ext[i]);

	std::map<std::string, std::string>::const_iterator	it = _types.find(ext);
	if (it != _types.end())
		return (it->second);

	return ("application/octet-stream");
}
