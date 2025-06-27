#pragma once
#include "httpprotocol.h"
#include "session.h"

namespace bee
{

class httpprotocol;
class httpsession_manager;

class httpsession : public session
{
public:
    httpsession(httpsession_manager* manager);
    virtual ~httpsession();

    virtual httpsession* dup() override;

    virtual void on_recv(size_t len) override;
    virtual void on_send(size_t len) override;

    void set_unfinished_protocol(httpprotocol* protocol) { _unfinished_protocol = protocol; }
    httpprotocol* get_unfinished_protocol() const { return _unfinished_protocol; }

protected:
    friend class httpsession_manager;
    uint64_t _requests = 0;
    httpprotocol* _unfinished_protocol = nullptr;
};

} // namespace bee