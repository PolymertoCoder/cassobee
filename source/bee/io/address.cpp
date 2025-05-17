#include "address.h"
#include "log.h"
#include <netdb.h>

namespace bee
{

bool address::lookup(std::vector<std::unique_ptr<address>>& addrs, const std::string& host, int family, int type, int protocol)
{
    addrinfo hints;
    addrinfo* results = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;

    std::string node;
    const char* service = nullptr;

    // 检查 ipv6address serivce
    if(!host.empty() && host[0] == '[')
    {
        const char* endipv6 = (const char*)memchr(host.data() + 1, ']', host.size() - 1);
        if(endipv6)
        {
            //TODO check out of range
            if(*(endipv6 + 1) == ':')
            {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.data() - 1);
        }
    }

    // 检查 node serivce
    if(node.empty())
    {
        service = (const char*)memchr(host.data(), ':', host.size());
        if(service)
        {
            if(!memchr(service + 1, ':', host.data() + host.size() - service - 1))
            {
                node = host.substr(0, service - host.data());
                ++service;
            }
        }
    }

    if(node.empty())
    {
        node = host;
    }

    int ret = getaddrinfo(node.data(), service, &hints, &results);
    if(ret != 0)
    {
        local_log << "address::lookup getaddrinfo failed, (" << host << ", " << family << ", " << type << ", " << protocol << ") ret=" << ret << "errstr=" << gai_strerror(ret);
        return false;
    }

    for(addrinfo* p = results; p != nullptr; p = p->ai_next)
    {
        address* addr = address_factory::create2<"general_address">(*p->ai_addr, p->ai_addrlen);
        addrs.emplace_back(addr);
    }
    
    freeaddrinfo(results);
    return !addrs.empty();
}

address* address::lookup_any(const std::string& host, int family, int type, int protocol)
{
    std::vector<std::unique_ptr<address>> result;
    if(lookup(result, host, family, type, protocol))
    {
        auto res = std::move(result[0]); // 转移所有权
        return res.release();
    }
    return nullptr;
}

} // namespace bee