#pragma once
#include "connection.h"
#include "objectpool.h"

namespace db
{

template<typename connection_type>
class connectionpool : public objectpool<connection_type>
{
public:
    void init(const std::string& ip, const std::string& user, const std::string& password, const std::string& db, uint16_t port, size_t minsize, size_t maxsize, TIMETYPE timeout, TIMETYPE max_idle_time)
    {
        _ip = ip;
        _port = port;
        _user = user;
        _password = password;
        _dbname = db;
        _minsize = minsize;
        _maxsize = maxsize;
        _timeout = timeout;
        _max_idle_time = max_idle_time;
    }

private:
    std::string _ip;
    uint16_t    _port = 0;
    std::string _user;
    std::string _password;
    std::string _dbname;

    size_t   _minsize       = 0;
    size_t   _maxsize       = 0;
    TIMETYPE _timeout       = 0;
    TIMETYPE _max_idle_time = 0;
};
    
}; // namespace db