#pragma once
#include <stdint.h>

#include "lock.h"
#include "octets.h"
#include "marshal.h"
#include "types.h"
#include "session_manager.h"

namespace bee
{

class address;
class session_manager;
struct event;

enum SESSION_STATE
{
    SESSION_STATE_NONE,
    SESSION_STATE_ACTIVE,
    SESSION_STATE_SENDING,
    SESSION_STATE_RECVING,
    SESSION_STATE_CLOSING
};

class session
{
public:
    session(session_manager* manager);
    virtual ~session();

    void clear();
    virtual session* dup();

    virtual void open();
    virtual void close();

    virtual void on_recv(size_t len);
    virtual void on_send(size_t len);

    void permit_recv();
    void permit_send();

    void forbid_recv();
    void forbid_send();

    FORCE_INLINE size_t max_rbuf_size() const { return _manager->_read_buffer_size;  }
    FORCE_INLINE size_t max_wbuf_size() const { return _manager->_write_buffer_size; }
    octets& rbuffer();
    octets& wbuffer();
    void clear_wbuffer();

    FORCE_INLINE void set_sid(SID sid) { _sid = sid; }
    FORCE_INLINE SID  get_sid() const { return _sid;}

    FORCE_INLINE address* get_peer() { return _peer; }
    FORCE_INLINE session_manager* get_manager() const { return _manager; }

    FORCE_INLINE void set_state(SESSION_STATE state) { _state = state; }
    FORCE_INLINE void set_event(event* ev) { _event = ev; }

    void activate(); // 更新会话的最后活跃时间
    bool is_timeout(TIMETYPE timeout) const; // 检查会话是否超时

protected:
    friend class session_manager;
    friend class httpsession_manager;
    friend class streamio_event;
    friend class sslio_event;
    SID _sid = 0;
    int _sockfd = 0;
    uint8_t _state = SESSION_STATE_NONE;
    address* _peer = nullptr;
    session_manager* _manager;

    bee::rwlock _locker;
    event* _event = nullptr;

    octetsstream _reados;
    octets _readbuf;

    size_t _write_offset = 0;
    octetsstream _writeos;
    octets _writebuf;

    TIMETYPE _last_active = 0; // 最后活跃时间
};

} // namespace bee