#include "cmysql.h"
#include "config.h"
#include "objectpool.h"
#include "systemtime.h"
#include <cppconn/connection.h>
#include <print>

namespace bee::mysql
{

connection* connection::get()
{
    return connection_pool::get_instance()->get_connection();
}

void connection::release(connection* conn)
{
    connection_pool::get_instance()->release_connection(conn);
}

bool connection::connect()
{
    auto cfg = config::get_instance();
    auto ip = cfg->get("mysql", "ip");
    auto user = cfg->get("mysql", "user");
    auto password = cfg->get("mysql", "password");
    auto db = cfg->get("mysql", "db");
    auto port = cfg->get<int>("mysql", "port");
    return connect(ip, user, password, db, port > 0 ? port : 3306);
}

bool connection::connect(const std::string& ip, const std::string& user, const std::string& password, const std::string& db, int port)
{
    try
    {
        _driver = sql::mysql::get_mysql_driver_instance();
        _conn = _driver->connect("tcp://" + ip + ":" + std::to_string(port), user, password);
        _conn->setSchema(db);
        return true;
    }
    catch (sql::SQLException& e)
    {
        // 处理连接错误
        std::println("connect to mysql failed: %s", e.what());
        return false;
    }
}

void connection::close()
{
    if(_conn)
    {
        _conn->close();
        delete _conn;
        _conn = nullptr;
    }
}

void connection::execute(const std::string& sql)
{
    dbstmt* stmt = _conn->createStatement();
    try
    {
        stmt->execute(sql);
        delete stmt;
    }
    catch(...)
    {
        delete stmt;
        throw;
    }
}

bool connection::execute_update(const std::string& sql)
{
    return true;
}

dbres* connection::execute_query(const std::string& sql)
{
    return _conn->createStatement()->executeQuery(sql);
}

dbpstmt* connection::prepare_statement(const std::string& sql)
{
    return _conn->prepareStatement(sql);
}

void connection::set_savepoint(const std::string& name)
{
    CHECK_BUG(!_savepoints.contains(name), throw exception("savepoint already exists", -1));
    _savepoints.emplace(name, _conn->setSavepoint(name));
}

void connection::release_savepoint(const std::string& name)
{
    // 释放所有保存点
    if(name.empty())
    {
        for(const auto& [name, savepoint] : _savepoints)
        {
            _conn->releaseSavepoint(savepoint);
        }
        _savepoints.clear();
        return;
    }

    // 释放指定保存点
    auto iter = _savepoints.find(name);
    CHECK_BUG(iter != _savepoints.end(), throw exception("savepoint not exists", -1));
    _conn->releaseSavepoint(iter->second);
    _savepoints.erase(iter);
}

void connection::rollback_to_savepoint(const std::string& name) // 不释放保存点
{
    auto iter = _savepoints.find(name);
    CHECK_BUG(iter != _savepoints.end(), throw exception("savepoint not exists", -1));
    _conn->rollback(iter->second);
}

void connection_pool::init()
{
    auto* cfg = config::get_instance();
    _ip = cfg->get("mysql", "ip");
    _port = cfg->get<int>("mysql", "port");
    _user = cfg->get("mysql", "user");
    _password = cfg->get("mysql", "password");
    _db = cfg->get("mysql", "db");
    _minsize = cfg->get<size_t>("mysql", "minsize");
    _maxsize = cfg->get<size_t>("mysql", "maxsize");
    _timeout =  cfg->get<TIMETYPE>("mysql", "timeout");
    _max_idle_time = cfg->get<TIMETYPE>("mysql", "max_idle_time");
    std::println("mysql connection pool init: %s:%d %s %s %s %zu %zu %zu", _ip.data(), _port, _user.data(), _password.data(), _db.data(), _minsize, _maxsize, _timeout);

    objectpool::init(_maxsize);

    for(size_t i = 0; i < _minsize; ++i)
    {
        auto [id, conn] = alloc();
        if(conn)
        {
            conn->connect(_ip, _user, _password, _db, _port);
            assert(conn->is_connected());
        }
    }
}

auto connection_pool::get_connection() -> connection*
{
    auto [id, conn] = alloc();
    if(conn)
    {
        if(!conn->is_connected())
        {
            if(!conn->connect(_ip, _user, _password, _db, _port))
            {
                free(id);
                return nullptr;
            }
        }
        conn->set_last_access(systemtime::get_time());
    }
    else
    {
        std::println("mysql connection pool exhausted.");
    }
    return conn;
}

void connection_pool::release_connection(connection* conn)
{
    if(!conn) return;
    if(conn->has_transaction())
    {
        conn->rollback();
    }
    conn->release_savepoint();

    conn->set_last_access(systemtime::get_time());
    free(conn);
}

void connection_pool::clear()
{
    objectpool::destroy();
    std::println("mysql connection pool clear");
}

} // namespace bee::mysql