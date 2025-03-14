#pragma once
#include "octets.h"

namespace bee
{

class ostringstream
{
public:
    ostringstream(const char* data, size_t size) : _buf(data, size) {}
    ostringstream(const std::string& str) : _buf(str) {}

private:
    octets _buf;
};

} // namespace bee