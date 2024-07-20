#pragma once
#include <map>
#include <utility>

#include "marshal.h"
#include "types.h"

class protocol : public marshal
{
public:
    protocol() { }
    protocol(const protocol&) {}
    virtual ~protocol() = default;

    virtual void run() {}
    virtual protocol* dup() const { return new protocol(*this); }

    void encode(octetsstream& os);
    void decode(octetsstream& os);

    static auto& get_map()
    {
        static std::map<PROTOCOLID, protocol*> _stubs;
        return _stubs;
    }
    static bool register_protocol(PROTOCOLID id, protocol* prot)
    {
        return get_map().emplace(id, prot).second;
    }
    static protocol* get_protocol(PROTOCOLID id)
    {
        if(auto iter = get_map().find(id); iter != get_map().end())
        {
            return iter->second->dup();
        }
        return nullptr;
    }

public:
    virtual octetsstream& pack(octetsstream& os)   override { return os; }
    virtual octetsstream& unpack(octetsstream& os) override { return os; }

protected:
    PROTOCOLID _type;
    uint32_t _priority;
    SID _sid;
};