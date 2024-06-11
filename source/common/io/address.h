#pragma once
#include <netinet/in.h>

class address
{
public:
    
};

class general_address : public address
{
public:
    
};

class ipv4_address : public address
{
public:
private:
    sockaddr_in _addr;
};

class ipv6_address : public address
{
public:
private:
    sockaddr_in _addr;
};

class unix_address : public address
{
public:
private:
    sockaddr_in _addr;
};