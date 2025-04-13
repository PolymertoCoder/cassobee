#pragma once
#include <unordered_map>
#include <utility>

#include "marshal.h"
#include "types.h"

namespace bee
{

class session;
class session_manager;
class ostringstream;

class protocol : public marshal
{
public:
    protocol() = default;
    protocol(PROTOCOLID type) : _type(type) { assert(register_protocol(type, this)); }
    protocol(const protocol&) = default;
    virtual ~protocol() = default;

    virtual PROTOCOLID get_type() const = 0;
    virtual size_t maxsize() const = 0;
    virtual protocol* dup() const = 0;
    virtual void run() = 0;
    virtual void dump(ostringstream& out) const;
    FORCE_INLINE virtual size_t thread_group_idx() const { return 0; }
    
    static bool size_policy(PROTOCOLID type, size_t size);
    static bool check_policy(PROTOCOLID type, size_t size, session_manager* manager);

    virtual void encode(octetsstream& os) const;
    static protocol* decode(octetsstream& os, session* ses);

public:
    FORCE_INLINE static auto& get_map()
    {
        static std::unordered_map<PROTOCOLID, protocol*> _stubs;
        return _stubs;
    }
    FORCE_INLINE static bool register_protocol(PROTOCOLID type, protocol* prot)
    {
        return get_map().emplace(type, prot).second;
    }
    FORCE_INLINE static protocol* get_protocol(PROTOCOLID type)
    {
        auto iter = get_map().find(type);
        return iter != get_map().end() ? iter->second->dup() : nullptr;
    }

protected:
    PROTOCOLID _type;
    uint32_t _priority;    
    SID _sid;
    session_manager* _manager;
};

ostringstream& operator<<(ostringstream& oss, const protocol& prot);

} // namespace bee