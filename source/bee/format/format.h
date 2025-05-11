#pragma once
#include "octets.h"
#include "stringfy.h"
#include "formatter.h"
#include "rpcdata.h"

namespace bee
{

template<typename... Args>
auto format(const char* fmt, Args&&... args)
{
    return format_string(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
auto format(std::format_string<Args...> fmt, Args&&... args)
{
    return std::format(fmt, std::forward<Args>(args)...);
}

class ostringstream
{
public:
    static constexpr size_t INITIAL_CAPACITY = 64;

    ostringstream() { _buf.reserve(INITIAL_CAPACITY); }
    ostringstream(const char* data, size_t size) : _buf(data, size) {}
    ostringstream(const std::string& str) : _buf(str) {}
    ostringstream(ostringstream&& other) noexcept = default;

    template<can_to_string T> ATTR_WEAK ostringstream& operator<<(const T& value)
    {
        if constexpr(std::is_same_v<T, char> || std::is_same_v<T, unsigned char>) {
            return *this << (char)value;
        } else {
            return *this << bee::to_string(value);
        }
    }

    ostringstream& operator<<(bool value);
    ostringstream& operator<<(char value);
    ostringstream& operator<<(const char* value);
    ostringstream& operator<<(const std::string& value);
    ostringstream& operator<<(ostringstream& (*manipulator)(ostringstream&));

    size_t size() const { return _buf.size(); }
    bool empty() const { return _buf.empty(); }
    void truncate(size_t size) { if(size < _buf.size()) _buf.fast_resize(size); }
    void reserve(size_t cap) { _buf.reserve(cap); }
    void clear() { _buf.clear();}
    std::string str() { return std::string(_buf.data(), _buf.size()); }
    const char* c_str();

private:
    octets _buf;
};

ostringstream& endl(ostringstream& oss);

} // namespace bee