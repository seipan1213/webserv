#include "ConfigParser.hpp"
#include "../utils.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "../utils.hpp"

ConfigParser::ConfigParser() : parser_line(), config(), server_config(), location_config(), nowBlock()
{
}

ConfigParser::~ConfigParser()
{
}
ConfigParser::ConfigParser(const ConfigParser& src)
{
	parser_line = src.parser_line;
	config = src.config;
	server_config = src.server_config;
	location_config = src.location_config;
	nowBlock = src.nowBlock;	
}
ConfigParser& ConfigParser::operator=(const ConfigParser& rhs)
{
	if (this != &rhs) {
		parser_line = rhs.parser_line;
		config = rhs.config;
		server_config = rhs.server_config;
		location_config = rhs.location_config;
		nowBlock = rhs.nowBlock;	
	}
	return (*this);
}
void ConfigParser::printParserLine()
{
	for (size_t i = 0; i < this->parser_line.size(); i++)
	{
		for (size_t j = 0; j < this->parser_line[i].size(); j++)
		{
			std::cout << this->parser_line[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

Config ConfigParser::readFile(const std::string &filepath)
{
	std::ifstream ifs(filepath.c_str());

	std::string line;
	// conf の一行ずつを読み込む
	while (std::getline(ifs, line))
	{
		// 空行やコメントアウト行は無視	

		ft::TrimWSP(line);
		if (line.length() == 0 || (line.length() != 0 && line[0] == '#'))
		{
			continue;
		}
		// 	server_name hoge_server; => [0]: "server_name" [1]: "hoge_server" [2]: ";"
		std::vector<std::string> params = this->splitLine(line);
		this->parser_line.push_back(params);
	}
	// 読み込んだ行のvectorを利用してクラスにセットする
	this->setConfigFromParseLine();
	return this->config;
}

std::vector<std::string> ConfigParser::splitLine(const std::string line)
{
	std::vector<std::string> params;
	size_t start = 0;
	size_t end = 0;

	while (line[start])
	{
		while (isspace(line[start]))
		{
			start++;
		}
		end = start;
		while (isprint(line[end]) && !(isspace(line[end]) || isParamDelimiter(line[end])))
		{
			end++;
		}
		if ((end - start) != 0)
		{
			params.push_back(line.substr(start, (end - start)));
		}
		if (isParamDelimiter(line[end]))
		{
			start = end;
			++end;
			params.push_back(line.substr(start, (end - start)));
			break;
		}
		start = end;
	}
	if (line[end])
	{
		throw std::invalid_argument("Error: should one param in a line");
	}
	return params;
}

bool ConfigParser::isParamDelimiter(int c)
{
	return ((c == '{') || (c == '}') || (c == ';'));
}

void ConfigParser::setConfigFromParseLine()
{
	this->nowBlock = ROOT;
	for (size_t i = 0; i < this->parser_line.size(); i++)
	{
		switch (this->nowBlock)
		{
		case ROOT:
			this->setConfigRoot(this->parser_line[i]);
			break;
		case SERVER:
			this->setConfigServer(this->parser_line[i]);
			break;
		case LOCATION:
			this->setConfigLocation(this->parser_line[i]);
			break;
		}
	}
}

void ConfigParser::setConfigRoot(std::vector<std::string> line)
{
	if (line[0] == "server" && line[1] == "{")
	{
		this->nowBlock = SERVER;
	}
	else
	{
		std::string msg = "unknown Directive (Root): " + line[0];
		throw std::invalid_argument(msg);
	}
}

void ConfigParser::setConfigServer(std::vector<std::string> line)
{
	if (line[0] == "server_name")
		this->setConfigServerName(SERVER, line);
	else if (line[0] == "listen")
		this->setConfigListen(SERVER, line);
	else if (line[0] == "client_max_body_size")
		this->setConfigClientMaxBodySize(SERVER, line);
	else if (line[0] == "error_page")
		this->setConfigErrorPage(SERVER, line);
	else if (line[0] == "location" && line[2] == "{")
	{
		this->nowBlock = LOCATION;
		if (line[1][0] != '/') {
			line[1].insert(0, 1, '/');
		}
		if (line[1][line[1].size() - 1] == '/') {
			line[1].erase(line[1].size() - 1);
		}
		this->location_config.setUri(line[1]);
	}
	else if (line[0] == "}")
	{
		this->config.addServerConfig(this->server_config);
		this->server_config = ServerConfig();
		this->nowBlock = ROOT;
	}
	else
	{
		std::string msg = "unknown Directive (Server): " + line[0];
		throw std::invalid_argument(msg);
	}
}

void ConfigParser::setConfigLocation(std::vector<std::string> line)
{
	if (line[0] == "index")
		this->setConfigIndex(LOCATION, line);
	else if (line[0] == "alias")
		this->setConfigAlias(LOCATION, line);
	else if (line[0] == "return")
		this->setConfigRedirect(LOCATION, line);
	else if (line[0] == "autoindex")
		this->setConfigAutoIndex(LOCATION, line);
	else if (line[0] == "allow_method")
		this->setConfigAllowMethod(LOCATION, line);
	else if (line[0] == "cgi_extension")
		this->setConfigCgiExtension(LOCATION, line);
	else if (line[0] == "upload_filepath")
		this->setConfigUploadFilepath(LOCATION, line);
	else if (line[0] == "}")
	{
		this->server_config.addLocationConfig(this->location_config.getUri(), this->location_config);
		this->location_config = LocationConfig();
		this->nowBlock = SERVER;
	}
	else
	{
		std::string msg = "unknown Directive (Location): " + line[0];
		throw std::invalid_argument(msg);
	}
}

void ConfigParser::setConfigServerName(E_BlockType block_type, std::vector<std::string> line)
{
	this->validateServerName(line);

	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		this->server_config.setServerName(line[1]);
		break;
	case LOCATION:
		throw std::exception();
		break;
	}
}

void ConfigParser::setConfigListen(E_BlockType block_type, std::vector<std::string> line)
{
	unsigned int port = this->validateListen(line);	
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		this->server_config.setListen(port);
		break;
	case LOCATION:
		throw std::exception();
		break;
	}
}

void ConfigParser::setConfigErrorPage(E_BlockType block_type, std::vector<std::string> line)
{
	this->validateErrorPage(line);
	std::string uri = line[line.size() - 2];

	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		for (std::vector<std::string>::iterator it = line.begin() + 1; (*it)[0] != '/'; ++it)
		{
			this->server_config.addErrorPage(ft::StrBase_to_UI_(*it, std::dec), uri);
		}
		break;
	case LOCATION:
		throw std::exception();
		break;
	}
}

void ConfigParser::setConfigClientMaxBodySize(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		this->server_config.setClientMaxBodySize(line[1]);
		break;
	case LOCATION:
		throw std::exception();
		break;
	}
}

void ConfigParser::setConfigAllowMethod(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		for (size_t i = 1; i < line.size() - 1; i++)
		{
			this->location_config.addAllowMethod(line[i]);
		}
		break;
	}
}

void ConfigParser::setConfigRedirect(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		for (size_t i = 1; i < line.size() - 1; i++)
		{
			this->location_config.addRedirect(atoi(line[1].c_str()), line[2]);
		}
		break;
	}
}

void ConfigParser::setConfigAlias(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		this->location_config.setAlias(line[1]);
		break;
	}
}

void ConfigParser::setConfigAutoIndex(E_BlockType block_type, std::vector<std::string> line)
{
	bool autoindex;
	if (line[1] == "on")
		autoindex = true;
	else if (line[1] == "off")
		autoindex = false;
	else
		throw std::exception();

	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		this->location_config.setAutoindex(autoindex);
		break;
	}
}

void ConfigParser::setConfigIndex(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		for (size_t i = 1; i < line.size() - 1; i++)
		{
			this->location_config.addIndex(line[i]);
		}
		break;
	}
}

void ConfigParser::setConfigCgiExtension(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		for (size_t i = 1; i < line.size() - 1; i++)
		{
			this->location_config.addCgiExtension(line[i]);
		}
		break;
	}
}

void ConfigParser::setConfigUploadFilepath(E_BlockType block_type, std::vector<std::string> line)
{
	switch (block_type)
	{
	case ROOT:
		throw std::exception();
		break;
	case SERVER:
		throw std::exception();
		break;
	case LOCATION:
		this->location_config.setUploadFilepath(line[1]);
		break;
	}
}

void ConfigParser::validateServerName(std::vector<std::string> line)
{
	if (!(line[0] == "server_name" && line[2] == ";" && line.size() == 3))
	{
		throw std::invalid_argument("Error: must fix server_name");
	}
}

void ConfigParser::validateErrorPage(std::vector<std::string> line)
{
	std::vector<std::string>::reverse_iterator it = line.rbegin();
	if (*it != ";" || (*(++it))[0] != '/' || line.size() < 4) {
		throw std::invalid_argument("Error: must fix error_page");
	}
}

unsigned int ConfigParser::validateListen(std::vector<std::string> line)
{
	if (!(line[2] == ";" && line.size() == 3))
	{
		throw std::invalid_argument("Error: must fix listen");
	}
	for (std::string::iterator it = line[1].begin(); it != line[1].end(); ++it) {
		if (!isdigit(*it)) {
			throw std::invalid_argument("Error: listen is not digit");
		}
	}
	unsigned int port = ft::StrBase_to_UI_(line[1], std::dec);
	if (port > 65535) {
		throw std::runtime_error("Error: listen port number is too high");
	}
	return (port);
}
/*



void ConfigParser::validateClientMaxBodySize(std::vector<std::string> line)
{
}

void ConfigParser::validateAllowMethod(std::vector<std::string> line)
{
}

void ConfigParser::validateRedirect(std::vector<std::string> line)
{
}

void ConfigParser::validateAlias(std::vector<std::string> line)
{
}

void ConfigParser::validateAutoIndex(std::vector<std::string> line)
{
}

void ConfigParser::validateIndex(std::vector<std::string> line)
{
}

void ConfigParser::validateCgiExtension(std::vector<std::string> line)
{
}

void ConfigParser::validateUploadFilepath(std::vector<std::string> line)
{
}*/