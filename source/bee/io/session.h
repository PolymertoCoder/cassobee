#pragma once
#include <stdint.h>

#include "lock.h"
#include "octets.h"
#include "marshal.h"
#include "types.h"

namespace bee
{

class address;
struct event;
class session_manager;

enum SESSION_STATE : uint8_t
{
    SESSION_STATE_NONE,
    SESSION_STATE_ACTIVE,
    SESSION_STATE_SENDING,
    SESSION_STATE_RECVING,
    SESSION_STATE_CLOSING,
};

enum SESSION_CLOSE_REASON : uint8_t
{
    SESSION_CLOSE_REASON_NONE,
    SESSION_CLOSE_REASON_LOCAL,
    SESSION_CLOSE_REASON_REMOTE,
    SESSION_CLOSE_REASON_TIMEOUT,
    SESSION_CLOSE_REASON_RESET,
    SESSION_CLOSE_REASON_ERROR,
    SESSION_CLOSE_REASON_EXCEPTION,
};

class session
{
public:
    session(session_manager* manager);
    virtual ~session();

    void clear();
    virtual session* dup();

    virtual void set_open();
    virtual void set_close(int reason = SESSION_CLOSE_REASON_LOCAL);

    virtual void on_recv(size_t len);
    virtual void on_send(size_t len);

    void permit_recv();
    void permit_send();

    void forbid_recv();
    void forbid_send();

    FORCE_INLINE size_t max_rbuf_size() const;
    FORCE_INLINE size_t max_wbuf_size() const;
    octets& rbuffer();
    octets& wbuffer();
    void clear_wbuffer();
    bool is_writeos_empty();

    FORCE_INLINE void set_sid(SID sid) { _sid = sid; }
    FORCE_INLINE SID  get_sid() const { return _sid;}

    FORCE_INLINE address* get_peer() { return _peer; }
    FORCE_INLINE session_manager* get_manager() const { return _manager; }

    FORCE_INLINE void set_state(SESSION_STATE state) { _state = state; }
    FORCE_INLINE void set_close_reason(SESSION_CLOSE_REASON reason) { _close_reason = reason; }
    FORCE_INLINE bool is_active() { return _state == SESSION_STATE_ACTIVE; }
    FORCE_INLINE bool is_close() { return _state == SESSION_STATE_CLOSING; }

    FORCE_INLINE void set_event(event* ev) { _event = ev; }

    void activate(); // 更新会话的最后活跃时间
    bool is_timeout(TIMETYPE timeout) const; // 检查会话是否超时

protected:
    virtual void close();

protected:
    friend class session_manager;
    friend class httpsession_manager;
    friend struct netio_event;
    friend struct streamio_event;
    friend struct sslio_event;

    SID _sid = 0;
    int _sockfd = 0;
    SESSION_STATE _state = SESSION_STATE_NONE;
    SESSION_CLOSE_REASON _close_reason = SESSION_CLOSE_REASON_NONE;
    TIMETYPE _last_active = 0; // 最后活跃时间

    address* _peer = nullptr;
    session_manager* _manager;

    bee::rwlock _locker;
    event* _event = nullptr;

    octetsstream _reados;
    octets _readbuf;

    size_t _write_offset = 0;
    octetsstream _writeos;
    octets _writebuf;
};

} // namespace bee