#pragma once
#include "connection.h"
#include "objectpool.h"

namespace cassobee::db
{

class connection_pool : public objectpool<mysql_connection>
{
public:
    void init();

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
    
}; // namespace cassobee::db