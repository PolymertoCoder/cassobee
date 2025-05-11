#include "format.h"

namespace bee
{

ostringstream& ostringstream::operator<<(bool value)
{
    _buf.append(value ? "true" : "false");
    return *this;
}

ostringstream& ostringstream::operator<<(char value)
{
    _buf.append(value);
    return *this;
}

ostringstream& ostringstream::operator<<(const char* value)
{
    _buf.append(value, std::strlen(value));
    return *this;
}

ostringstream& ostringstream::operator<<(const std::string& value)
{
    _buf.append(value.data(), value.size());
    return *this;
}

ostringstream& ostringstream::operator<<(ostringstream& (*manipulator)(ostringstream&))
{
    return manipulator(*this);
}

const char* ostringstream::c_str()
{
    if(_buf.empty() || _buf.back() != '\0')
    {
        _buf.append('\0');
    }
    return _buf;
}

ostringstream& endl(ostringstream& oss)
{
    oss << "\n";
    return oss;
}

} // namespace bee