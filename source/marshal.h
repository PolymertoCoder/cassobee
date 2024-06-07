#pragma once

#include "octets.h"

class octetsstream;

class marshal
{
public:
    virtual octetsstream& pack(octetsstream&)   = 0;
    virtual octetsstream& unpack(octetsstream&) = 0;
};

class octetsstream
{
public:
    octetsstream(const octets& oct) : _data(oct) {}
    octetsstream(const octets& oct, size_t size) : _data(oct, size) {}

private:
    octets _data;
    size_t _pos = 0;
    size_t _trans;
};