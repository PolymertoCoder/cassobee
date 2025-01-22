#pragma once
#include <string>

namespace db
{

class mysql_connection
{
public:
    bool connect(const std::string& ip, const std::string& user, const std::string& password, const std::string& db, int port);
    bool update(const std::string& sql);
    bool query(const std::string& sql);
    bool get_result(const std::string& sql);
    auto get_value(const int field_index) -> std::string; 
    void close();
    bool getResult();
private:

};

} // namespace db