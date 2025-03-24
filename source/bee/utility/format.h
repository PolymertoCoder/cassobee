#pragma once
#include "octets.h"
#include <format>
#include <type_traits>

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
    ostringstream() = default;
    ostringstream(const char* data, size_t size) : _buf(data, size) {}
    ostringstream(const std::string& str) : _buf(str) {}

    template<typename T>
    requires std::is_arithmetic_v<T>
    ostringstream& operator<<(const T& value)
    {
        auto str = std::to_string(value);
        _buf.append(str.data(), str.size());
        return *this;
    }

    ostringstream& operator<<(const char* value)
    {
        _buf.append(value, std::strlen(value));
        return *this;
    }

    ostringstream& operator<<(const std::string& value)
    {
        _buf.append(value.data(), value.size());
        return *this;
    }

    bool empty()
    {
        return _buf.empty();
    }

    void clear()
    {
        _buf.clear();
    }

    std::string str()
    {
        return std::string(c_str());
    }

    const char* c_str()
    {
        if(_buf.begin()[_buf.size() - 1] != '\0')
        {
            _buf.append('\0');
        }
        return _buf;
    }

private:
    octets _buf;
};

} // namespace bee