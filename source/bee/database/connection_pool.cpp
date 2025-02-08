#include "connection_pool.h"
#include "config.h"

namespace cassobee::db
{

void connection_pool::init()
{
    auto* cfg = config::get_instance();
    _ip = cfg->get("connection_pool", "ip");
    _port = cfg->get<int>("connection_pool", "port");
    _user = cfg->get("connection_pool", "user");
    _password = cfg->get("connection_pool", "password");
    _dbname = cfg->get("connection_pool", "dbname");
    _minsize = cfg->get<size_t>("connection_pool", "minsize");
    _maxsize = cfg->get<size_t>("connection_pool", "maxsize");
    _timeout =  cfg->get<TIMETYPE>("connection_pool", "timeout");
    _max_idle_time = cfg->get<TIMETYPE>("connection_pool", "max_idle_time");
}

} // namespace cassobee::db