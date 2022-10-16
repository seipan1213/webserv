#pragma once
#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <string>
#include <limits>
#include <cstddef>
#include <vector>
#include <map>
#include <sstream>
#include <exception>
#include <stack>
#include <iostream>
#include <algorithm>

namespace ft {

    const static std::string    DELIM = "\n";
    const static std::string    BREAK = "\n\n";
    const static std::string    SP = " ";
    const static std::string    CN = ":";
    const static std::string    HTTPV = "HTTP/1.1";
    const static size_t         MAXBUFFER = 8000;
    const static size_t         MAXHEADERS = 100;

    class HTTPRequest {
    public:
        typedef std::map<std::string, std::string>  header_type;
        typedef std::pair<std::string, std::string> header_value;

    private:
        enum            ParseStatus { requestLine, headerFields, readChunks, readStraight, complete };

        int             _responseCode;
        ParseStatus     _parseStatus;
        std::string     _requestMethod;
        std::string     _requestURI;
        std::string     _HTTPv;
        header_type     _headerFields;
        header_value    _currentHeader;
        unsigned int    _contentLength;
        unsigned int    _readBytes;
        std::string     _body;
        std::string     _save;
        std::vector<std::string> _validMethods;



    public:
        HTTPRequest();
        //HTTPRequest(const HTTPRequest& src);
        //HTTPRequest& operator=(const HTTPRequest& rhs);
        ~HTTPRequest();

        int     Parse(const std::string& request);
        bool    HTTPRequestComplete();
    
        const std::string&  GetRequestMethod();
        const std::string&  GetRequestURI();
        const std::string&  GetHTTPv();
        const header_type&  GetHeaderFields();
        const std::string&  GetBody();

    private:


        void        _parseRequestLine();
        bool        _parseHeaderFields();
        void        _toLower(std::string& str);
        void        _removeWSP(std::string& str);
        bool        _validateHeader();
        void        _readBody();
        void        _decideReadType();
        unsigned int _strToBase(const std::string& str, std::ios_base& (*base)(std::ios_base&));
        void        _throw(int responseCode, const std::string& message);
        void        _resetCurrentHeader();

    public:
        void    PrintRequest();
        void    PrintBody();
    };
}


#endif