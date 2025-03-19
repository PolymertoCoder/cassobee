#pragma once
#include "octets.h"
#include <type_traits>

namespace bee
{

class ostringstream
{
public:
    ostringstream() = default;
    ostringstream(const char* data, size_t size) : _buf(data, size) {}
    ostringstream(const std::string& str) : _buf(str) {}

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    ostringstream& operator<<(const T& value)
    {
        _buf.append(&value, sizeof(T));
        return *this;
    }

    ostringstream& operator<<(const char* value)
    {
        _buf.append(value, std::strlen(value));
        return *this;
    }

    ostringstream& operator<<(const std::string& value)
    {
        _buf.append(value.c_str(), value.size());
        return *this;
    }

    std::string str() const
    {
        return std::string(_buf.data(), _buf.size());
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