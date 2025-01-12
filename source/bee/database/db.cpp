#include "db.h"
#include "common.h"

namespace db
{

bool table::find(const octets& key, octets& value)
{
    if(!_conn) return false;
    std::string sql = format_string("SELECT value FROM %s WHERE key = ?", _name.data());
    auto stmt = _conn->prepare_statement(sql);
    if(!stmt)
    {
        throw dbexeception("table::find, failed to prepare statement.");
    }

    _conn->execute_query("");
    return true;
}

bool table::insert(const octets& key, octets& value)
{
    if(!_conn) return false;


}

} // namespace db