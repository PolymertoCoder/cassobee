#pragma once
#include "connection.h"
#include "octets.h"
#include <stdexcept>

namespace db
{

class dbexeception : public std::runtime_error
{
public:
    dbexeception(const std::string& msg) : std::runtime_error(msg), _msg(msg) {}
    virtual const char* what() const noexcept override { return _msg.data(); }
private:
    std::string _msg;
};

class table
{
public:
    explicit table(const std::string& name) : _name(name) {}
    virtual bool find(const octets& key, octets& value);
    virtual bool insert(const octets& key, octets& value);

    void set_connection(connection* conn) { _conn = conn; }

private:
    std::string _name;
    connection* _conn = nullptr;
};

} // namespace db