#pragma once
#include "common.h"
#include "lock.h"
#include "objectpool.h"
#include "traits.h"
#include "types.h"
#include <cstdint>
#include <cstring>
#include <iterator>
#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <memory>
#include <vector>

namespace bee::mysql
{

using dbstmt   = sql::Statement;
using dbpstmt  = sql::PreparedStatement;
using dbres    = sql::ResultSet;
using dbdriver = sql::mysql::MySQL_Driver;
using dbconn   = sql::Connection;
using dbsavept = sql::Savepoint;

// 异常类
class exception : public std::runtime_error
{
public:
    int code = 0;
    exception(const std::string& msg, int code) : std::runtime_error(msg), code(code) {}
};

// 类型标签
template<typename T> struct type_tag {};

// 字段基类
class field_base
{
public:
    std::string name;
    struct
    {
        bool is_primary_key : 1;
        bool is_auto_increment : 1;
        bool is_nullable : 1;
        bool is_unique : 1;
        bool is_index : 1;
        bool is_foreign_key : 1;
    } flag;

    field_base(std::string name) : name(std::move(name)) {}
};

// 具体字段类型特化
template<typename T, size_t Size = sizeof(T)>
class field : public field_base
{
public:
    using value_type = T;

    field(std::string name) : field_base(std::move(name)) {}

    static const char* type()
    {
        if constexpr(std::is_same_v<T, int>) return "INT";
        if constexpr(std::is_same_v<T, double>) return "DOUBLE";
        if constexpr(std::is_same_v<T, int64_t>) return "BIGINT";
        if constexpr(std::is_same_v<T, std::string>) return "VARCHAR";
        if constexpr(std::is_same_v<T, char[Size]>) return "VARCHAR";
        if constexpr(std::is_same_v<T, std::vector<uint8_t>>) return "BLOB";
        else static_assert(false, "unsupported field type");
    }
    static void bind(dbpstmt* stmt, int index, const T& value);
    static void extract(dbres* res, int index, T& value);
};

// field int
template<>
void field<int>::bind(dbpstmt* stmt, int index, const int& value)
{
    stmt->setInt(index, value);
}
template<>
void field<int>::extract(dbres* res, int index, int& value)
{
    value = res->getInt(index);
}

// field int64_t
template<>
void field<int64_t>::bind(dbpstmt* stmt, int index, const int64_t& value)
{
    stmt->setInt64(index, value);
}
template<>
void field<int64_t>::extract(dbres* res, int index, int64_t& value)
{
    value = res->getInt64(index);
}

// field double
template<>
void field<double>::bind(dbpstmt* stmt, int index, const double& value)
{
    stmt->setDouble(index, value);
}
template<>
void field<double>::extract(dbres* res, int index, double& value)
{
    value = res->getDouble(index);
}

// field std::string
template<>
void field<std::string>::bind(dbpstmt* stmt, int index, const std::string& value)
{
    stmt->setString(index, value);
}

template<>
void field<std::string>::extract(dbres* res, int index, std::string& value)
{
    value = res->getString(index);
}

// field char[]
template<size_t Size>
class field<char[Size], Size> : public field_base
{
public:
    static void bind(dbpstmt* stmt, int index, const char* value)
    {
        stmt->setString(index, value);
    }
    static void bind(dbpstmt* stmt, int index, const std::string& value)
    {
        stmt->setString(index, value);
    }
    static void extract(dbres* res, int index, char* value)
    {
        auto resstr = res->getString(index);
        strncpy(value, resstr.c_str(), resstr->size());
        value[resstr->size()] = '\0';
    }
    static void extract(dbres* res, int index, std::string& value)
    {
        value = res->getString(index);
    }
};

// field std::vector<char>
template<>
class field<std::vector<uint8_t>> : public field_base
{
public:
    static void bind(dbpstmt* stmt, int index, const std::vector<uint8_t>& value)
    {
        std::istringstream iss(std::string(value.begin(), value.end()));
        stmt->setBlob(index, &iss);
    }
    static void extract(dbres* res, int index, std::vector<uint8_t>& value)
    {
        std::istream* blob = res->getBlob(index);
        value.assign(std::istream_iterator<char>(*blob), std::istream_iterator<char>());
    }
};

//---------------------- 数据库连接管理 ----------------------
class connection
{
public:
    connection() = default;
    ~connection() { close(); }

    static connection* get();
    static void release(connection* conn);
    bool connect();
    bool connect(const std::string& ip, const std::string& user, const std::string& password, const std::string& db, int port = 3306);
    void close();

    void execute(const std::string& sql);
    bool execute_update(const std::string& sql);
    dbres* execute_query(const std::string& sql);
    dbpstmt* prepare_statement(const std::string& sql);

    // 事务
    FORCE_INLINE void begin_transaction() { _conn->setAutoCommit(false); }
    FORCE_INLINE bool has_transaction() const { return !_conn->getAutoCommit();}
    FORCE_INLINE void commit() { _conn->commit(); _conn->setAutoCommit(true); }
    FORCE_INLINE void rollback() { _conn->rollback(); _conn->setAutoCommit(true); }
    FORCE_INLINE void set_autocommit(bool mode) { _conn->setAutoCommit(mode); }
    void set_savepoint(const std::string& name);
    void release_savepoint(const std::string& name = "");
    void rollback_to_savepoint(const std::string& name); // 不释放保存点

    FORCE_INLINE void set_last_access(TIMETYPE time) { _last_access = time; }
    FORCE_INLINE auto get_last_access() -> TIMETYPE { return _last_access; }
    FORCE_INLINE auto get_handle() const { return _conn; }
    FORCE_INLINE bool is_connected() const { return _conn && !_conn->isClosed(); }
    FORCE_INLINE void reconnect() { close(); connect(); }

    template<typename... Args>
    bool execute_update(const std::string& sql, Args&&... args)
    {
        if(!_conn) return false;
        auto stmt = _conn->prepareStatement(sql);
        int index = 1;
        (field<Args>(std::to_string(index++)).bind(stmt, index, args), ...);
        return stmt->executeUpdate();
    }

    template<typename... Args>
    std::unique_ptr<dbres> execute_query(const std::string& sql, Args&&... args)
    {
        if(!_conn) return nullptr;
        auto stmt = _conn->prepareStatement(sql);
        int index = 1;
        (field<Args>(std::to_string(index++)).bind(stmt, index, args), ...);
        return std::unique_ptr<dbres>(stmt->executeQuery());
    }

    template<typename... Args>
    bool execute_update(const std::string& sql, const std::tuple<Args...>& args)
    {
        if(!_conn) return false;
        auto stmt = _conn->prepareStatement(sql);
        int index = 1;
        std::apply([&](const auto&... args) { (field<Args>(std::to_string(index++)).bind(stmt, index, args), ...); }, args);
        return stmt->executeUpdate();
    }

    template<typename... Args>
    std::unique_ptr<dbres> execute_query(const std::string& sql, const std::tuple<Args...>& args)
    {
        if(!_conn) return nullptr;
        auto stmt = _conn->prepareStatement(sql);
        int index = 1;
        std::apply([&](const auto&... args) { (field<Args>(std::to_string(index++)).bind_param(stmt, index, args), ...); }, args);
        return std::unique_ptr<dbres>(stmt->executeQuery());
    }

private:
    TIMETYPE  _last_access = 0;
    dbdriver* _driver = nullptr;
    dbconn*   _conn   = nullptr;
    std::map<std::string, dbsavept*> _savepoints;
};

// ---------------------- 流式操作 ----------------------
class insert_stream_base
{
protected:
    dbpstmt* _stmt = nullptr;
    int _param_count = 0;

    template<typename T>
    void _bind(const T& value)
    {
        field<T>::bind(_stmt, ++_param_count, value);
    }

public:
    explicit insert_stream_base(dbpstmt* stmt) : _stmt(stmt) {}
    virtual ~insert_stream_base() = default;

    void execute()
    {
        _stmt->executeUpdate();
        delete _stmt;
        _stmt = nullptr;
    }
};

template<typename... Fields>
class insert_stream : public insert_stream_base
{
public:
    insert_stream(dbpstmt* stmt) : insert_stream_base(stmt) {}

    insert_stream& operator <<(const std::tuple<Fields...>& args)
    {
        std::apply([&](const auto&... args) { (_bind(args), ...); }, args);
        return *this;
    }
    insert_stream& operator <<(const Fields&... args)
    {
        (_bind(args), ...);
        return *this;
    }
    insert_stream& operator <<(void(*)(insert_stream&))
    {
        this->execute();
        return *this;
    }
};

inline void execute(insert_stream_base& stream)
{
    stream.execute();
}

class select_stream_base
{
protected:
    dbpstmt* _stmt = nullptr;
    dbres* _res = nullptr;
    int _column_idx = 0;

public:
    select_stream_base(dbpstmt* stmt) : _stmt(stmt), _res(stmt->executeQuery()) {}

    ~select_stream_base()
    {
        delete _res;
        delete _stmt;
    }

    bool next() { return _res->next(); }

    template<typename T>
    void _extract(T& value)
    {
        field<T>::extract(_res, ++_column_idx, value);
        _column_idx %= _res->getMetaData()->getColumnCount();
    }
};

template<typename... Fields>
class select_stream : public select_stream_base
{
public:
    select_stream(dbpstmt* stmt) : select_stream_base(stmt) {}

    select_stream& operator >>(std::tuple<Fields...>& args)
    {
        std::apply([&](auto&... args) { (_extract(args), ...); }, args);
        return *this;
    }
    select_stream& operator >>(Fields&... args)
    {
        (_extract(args), ...);
        return *this;
    }
};

template<typename... Fields>
class table
{
    connection* _conn = nullptr;
    std::string _name;
    bee::typelist<Fields...> _fields;

    template<typename T>
    static std::string build_column_def(const field<T>& f)
    {
        std::string def = f.name + " " + field<T>::type();
        if(f.flag.is_primary_key) def += " PRIMARY KEY";
        if(f.flag.is_auto_increment) def += " AUTO_INCREMENT";
        if(f.flag.is_nullable) def += " NOT NULL";
        if(f.flag.is_unique) def += " UNIQUE";
        if(f.flag.is_index) def += " INDEX";
        if(f.flag.is_foreign_key) def += " FOREIGN KEY";
        return def;
    }

public:
    table(connection* conn, const std::string& name) : _conn(conn), _name(name) {}

    void create()
    {
        std::string sql = "CREATE TABLE IF NOT EXISTS `";
        sql += _name;
        sql += "` (";

        [&]<typename... Ts>(bee::typelist<Ts...>)
        {
            ((sql += build_column_def(Ts{}) + ", "), ...);
        }(_fields);

        sql.resize(sql.size() - 2); // remove the last comma
        sql += ")";

        _conn->execute(sql);
    }

    auto insert()
    {
        std::string sql = "INSERT INTO `";
        sql += _name;
        sql += "` VALUES (";
        sql.append(sizeof...(Fields) - 1, '?').append(".?)");
        return insert_stream<Fields...>(_conn->prepare_statement(sql));
    }

    auto select(const std::string& where = "")
    {
        std::string sql = "SELECT * FROM `";
        sql += _name;
        sql += "` ";
        sql += where;
        return select_stream<Fields...>(_conn->prepare_statement(sql));
    }
};

class connection_pool : public objectpool<connection>,
                        public singleton_support<connection_pool>
{
public:
    void init();
    auto get_connection() -> connection*;
    void release_connection(connection* conn);
    void clear();

private:
    bee::mutex _locker;
    std::string _ip;
    uint16_t    _port = 0;
    std::string _user;
    std::string _password;
    std::string _db;

    size_t   _minsize       = 0;
    size_t   _maxsize       = 0;
    TIMETYPE _timeout       = 0;
    TIMETYPE _max_idle_time = 0;
};

} // namespace bee::mysql