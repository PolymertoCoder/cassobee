#pragma once
#include <unordered_map>
#include <utility>

#include "marshal.h"
#include "types.h"

class protocol : public marshal
{
public:
    protocol() {}
    protocol(const protocol&) {}
    virtual ~protocol() = default;

    virtual void run() {};
    virtual protocol* dup() = 0;

    static bool check_policy(PROTOCOLID id, size_t size);

    static void encode(octetsstream& os);
    static void decode(octetsstream& os);

    FORCE_INLINE PROTOCOLID get_type() const { return _type; }
    FORCE_INLINE virtual size_t thread_group_idx() const { return 0; }

public:
    FORCE_INLINE static auto& get_map()
    {
        static std::unordered_map<PROTOCOLID, protocol*> _stubs;
        return _stubs;
    }
    FORCE_INLINE static bool register_protocol(PROTOCOLID id, protocol* prot)
    {
        return get_map().emplace(id, prot).second;
    }
    FORCE_INLINE static protocol* get_protocol(PROTOCOLID id)
    {
        auto iter = get_map().find(id);
        return iter != get_map().end() ? iter->second->dup() : nullptr;
    }

protected:
    PROTOCOLID _type;
    uint32_t _priority;
    size_t maxsize;
    SID _sid;
};