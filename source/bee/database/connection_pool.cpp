#include "connection_pool.h"
#include "config.h"

void connection_pool::init()
{
    auto* cfg = config::get_instance();
    const std::string& ip, const std::string& user, const std::string& password, const std::string& db, uint16_t port, size_t minsize, size_t maxsize, TIMETYPE timeout, TIMETYPE max_idle_time
    _ip = cfg->get("connection_pool", "ip");
    _port = cfg->get<int>("connection_pool", "port");
    _user = user;
    _password = password;
    _dbname = db;
    _minsize = minsize;
    _maxsize = maxsize;
    _timeout = timeout;
    _max_idle_time = max_idle_time;
}