#pragma once
#include "common.h"
#include "connection.h"
#include "octets.h"
#include <stdexcept>
#include <unordered_map>

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

class table_manager : public singleton_support<table_manager>
{
public:
    bool add_table(const std::string& table_name, table* table);
    bool del_table(const std::string& table_name);

private:
    std::unordered_map<std::string, table*> _tables;
};

} // namespace db