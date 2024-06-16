#pragma once
#include "marshal.h"
#include <map>
#include <sys/types.h>

class protocol : public marshal
{
public:
    protocol() { }
    protocol(const protocol&) {}
    virtual ~protocol() = default;

    virtual void run() {}
    virtual protocol* dup() const { return new protocol(*this); }

    static bool register_protocol(PROTOCOLID id, protocol* prot)
    {
        return _stubs.emplace(id, prot).second;
    }
    static protocol* get_protocol(PROTOCOLID id)
    {
        if(auto iter = _stubs.find(id); iter != _stubs.end())
        {
            return iter->second->dup();
        }
        return nullptr;
    }

public:
    virtual octetsstream& pack(octetsstream& os)   override { return os; }
    virtual octetsstream& unpack(octetsstream& os) override { return os; }

protected:
    static std::map<PROTOCOLID, protocol*> _stubs;

    PROTOCOLID _type;
    SID _sid;
};