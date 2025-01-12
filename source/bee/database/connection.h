#pragma once
#include <string>

namespace db
{

class connection
{
public:
    virtual ~connection() = default;
    virtual bool connect(const std::string& ip, const std::string& user, const std::string& password, const std::string& db, int port) = 0;
    virtual bool execute_update(const std::string& sql) = 0;
    virtual bool execute_query(const std::string& sql) = 0;
    virtual bool prepare_statement(const std::string& sql) = 0;
    virtual void close() = 0;
    virtual bool getResult();


private:

};

class mysql_connection : public connection
{

};

} // namespace db