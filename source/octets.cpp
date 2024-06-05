#include "octets.h"

octets::octets(const char* data)
{

}

octets::octets(const char* data, size_t size)
{

}

octets::octets(const std::string& str)
{

}

octets::octets(const std::string_view& str)
{

}

octets::octets(const octets& oct)
{

}

octets::octets(octets&& oct)
{

}

octets& octets::operator=(const char* rhs)
{

}

octets& octets::operator=(const octets& rhs)
{
    
}

octets& octets::operator=(octets&& rhs)
{
    
}

octets& octets::operator=(const std::string& rhs)
{
    
}

octets& octets::operator=(const std::string_view& rhs)
{
    
}

octets::~octets()
{
    
}

void octets::append(const char* data, size_t size)
{
    
}

void octets::reserve(size_t size)
{
    
}

octets octets::dup()
{
    
}

void octets::create(const char* data, size_t size)
{
    if(_data)
    {
        delete[] _data;
    }
    _data = new char[size];
    _size = 0;
    _cap  = size;
}

void octets::destroy()
{
    if(_data)
    {
        delete[] _data;
        _data = nullptr;
    }
    _size = 0;
    _cap  = 0;
}