#include "db.h"
#include "common.h"

namespace cassobee::db
{

bool table::find(const octets& key, octets& value)
{
    if(!_conn)
    {
        throw dbexeception("table::find, connection is null");
    }
    std::string sql = format_string("SELECT value FROM %s WHERE key = ?", _name.data());
    // auto stmt = _conn->prepare_statement(sql);
    // if(!stmt)
    // {
    //     throw dbexeception("table::find, failed to prepare statement");
    // }

    // _conn->execute_query("");
    return true;
}

bool table::insert(const octets& key, octets& value)
{
    if(!_conn)
    {
        throw dbexeception("table::insert, connection is null");
    }

    
}

bool table_manager::add_table(const std::string& table_name, table* table)
{
    if(!table) return false;
    return _tables.emplace(table_name, table).second;
}

bool table_manager::del_table(const std::string& table_name)
{
    return _tables.erase(table_name);
}

} // namespace cassobee::db