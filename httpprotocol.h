#ifndef HTTPPROTOCOL_H
#define HTTPPROTOCOL_H

#include <string>
#include <map>

class HttpProtocol {
public:
    HttpProtocol();
    ~HttpProtocol();

    std::string parseRequest(const std::string& rawRequest);
    std::string buildResponse(const std::string& content, int statusCode);

private:
    std::map<std::string, std::string> headers;
    std::string method;
    std::string uri;
    std::string version;
};

#endif // HTTPPROTOCOL_H 