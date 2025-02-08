#pragma once
#include <string>

namespace cassobee::db
{

class mysql_connection
{
public:
    bool connect(const std::string& ip, const std::string& user, const std::string& password,
                 const std::string& db, int port = 3306);
    void close();

    bool update(const std::string& sql);
    bool query(const std::string& sql);
    bool get_result();
    auto get_field(int index) -> std::string;
    auto get_value(const int field_index) -> std::string;

    // 事务
    bool transaction();
    bool commit();
    bool rollback();

private:
    
};

} // namespace cassobee::db