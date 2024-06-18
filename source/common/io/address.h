#pragma once
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "common.h"
#include "factory.h"

class address
{
public:
    enum AddressType
    {
        GENERAL,
        INET,
        INET6,
        UNIX
    };

    virtual ~address();
    virtual sockaddr* addr() = 0;
    virtual socklen_t len() = 0;
    virtual int family() = 0;
    virtual address* dup() = 0;
    virtual std::string to_string() = 0;
};

class general_address : public address
{
public:
    general_address(sockaddr addr, socklen_t len)
        : _addr(addr), _len(len) {}
    general_address(const general_address& rhs)
    {
        _addr = rhs._addr;
        _len = rhs._len;
    }
    virtual sockaddr* addr() override { return &_addr; };
    virtual socklen_t len() override { return _len; };
    virtual int family() override { return _addr.sa_family; };
    virtual general_address* dup() override { return new general_address(*this); }
    virtual std::string to_string() override
    {
        return format_string("general_address family:%d sa_data:%s socklen:%d", family(), _addr.sa_data, _len);
    }

public:
    sockaddr _addr;
    socklen_t _len;
};

class ipv4_address : public address
{
public:
    ipv4_address(sockaddr_in addr, socklen_t len) : _addr(addr), _len(len) {}
    ipv4_address(const char* ip, uint16_t port)
    {
        _addr.sin_family = AF_INET;
        _addr.sin_port = htobe16(port);
        inet_pton(AF_INET, ip, &_addr.sin_addr);
        _len = sizeof(sockaddr_in);
    }
    ipv4_address(const ipv4_address& rhs)
    {
        _addr = rhs._addr;
        _len = rhs._len;
    }
    virtual sockaddr* addr() override { return (sockaddr*)&_addr; };
    virtual socklen_t len() override { return _len; };
    virtual int family() override { return AF_INET; };
    virtual ipv4_address* dup() override { return new ipv4_address(*this); }
    virtual std::string to_string() override
    {
        return format_string("ipv4_address ip:%s port:%d", ip().data(), port());
    }

    std::string ip()
    {
        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &_addr.sin_addr, ip_buf, sizeof(ip_buf));
        return ip_buf;
    }
    uint16_t port() { return ntohs(_addr.sin_port); }

public:
    sockaddr_in _addr;
    socklen_t _len;
};

class ipv6_address : public address
{
public:
    ipv6_address(sockaddr_in6 addr, socklen_t len) : _addr(addr), _len(len) {}
    ipv6_address(const char* ip, uint16_t port)
    {
        _addr.sin6_family = AF_INET6;
        _addr.sin6_port = htobe16(port);
        inet_pton(AF_INET6, ip, &_addr.sin6_addr);
        _len = sizeof(sockaddr_in);
    }
    ipv6_address(const ipv6_address& rhs)
    {
        _addr = rhs._addr;
        _len = rhs._len;
    }
    virtual sockaddr* addr() override { return (sockaddr*)&_addr; };
    virtual socklen_t len() override { return _len; };
    virtual int family() override { return AF_INET6; };
    virtual ipv6_address* dup() override { return new ipv6_address(*this); }
    virtual std::string to_string() override
    {
        return format_string("ipv6_address ip:%s port:%d", ip().data(), port());
    }

    std::string ip()
    {
        char ip_buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &_addr.sin6_addr, ip_buf, sizeof(ip_buf));
        return ip_buf;
    }
    uint16_t port() { return ntohs(_addr.sin6_port); }

public:
    sockaddr_in6 _addr;
    socklen_t _len;
};

class unix_address : public address
{
public:
    unix_address(sockaddr_un addr, socklen_t len) : _addr(addr), _len(len) {}
    unix_address(const char* path)
    {
        _addr.sun_family = AF_UNIX;
        strncpy(_addr.sun_path, path, sizeof(_addr.sun_path) - 1);
        _len = sizeof(_addr.sun_family) + strlen(_addr.sun_path) + 1;
    }
    unix_address(const unix_address& rhs)
    {
        _addr = rhs._addr;
        _len = rhs._len;
    }
    virtual sockaddr* addr() override { return (sockaddr*)&_addr; };
    virtual socklen_t len() override { return _len; };
    virtual int family() override { return AF_UNIX; };
    virtual unix_address* dup() override { return new unix_address(*this); }
    virtual std::string to_string() override
    {
        return format_string("unix_address path:%s", path().data());
    }

    std::string path() { return _addr.sun_path; }

public:
    sockaddr_un _addr;
    socklen_t _len;
};

using address_factory = factory_template<address, address::AddressType>;
register_product(address_factory, address::AddressType::GENERAL, general_address, sockaddr, socklen_t);
register_product(address_factory, address::AddressType::INET, ipv4_address, const char*, uint16_t);
register_product(address_factory, address::AddressType::INET6, ipv6_address, const char*, uint16_t);
register_product(address_factory, address::AddressType::UNIX, unix_address, const char*);