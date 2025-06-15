#pragma once
#include "octets.h"
#include "stringfy.h"
#include "formatter.h"
#include "rpcdata.h"

namespace bee
{

class ostringstream
{
public:
    static constexpr size_t INITIAL_CAPACITY = 64;

    ostringstream() { _buf.reserve(INITIAL_CAPACITY); }
    ostringstream(const char* data, size_t size) : _buf(data, size) {}
    ostringstream(const std::string& str) : _buf(str) {}
    ostringstream(ostringstream&& other) noexcept = default;

    template<can_to_string T> ostringstream& operator<<(const T& value)
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

    octets& data() { return _buf; }
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

template<typename... Args>
std::string join(const std::string& sep, const Args&... args)
{
    thread_local bee::ostringstream oss;
    oss.clear();
    size_t count = 0;
    static constinit size_t size = sizeof...(Args);
    ((oss << bee::to_string(args) << (++count < size ? sep : "")), ...);
    return oss.str();
}

} // namespace bee