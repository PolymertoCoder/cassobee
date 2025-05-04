#pragma once
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

    virtual void set_open() override;
    virtual void set_close() override;

    virtual void on_recv(size_t len) override;
    virtual void on_send(size_t len) override;

    void set_unfinished_protocol(httpprotocol* protocol) { _unfinished_protocol = protocol; }
    httpprotocol* get_unfinished_protocol() const { return _unfinished_protocol; }

private:
    friend class httpsession_manager;
    httpprotocol* _unfinished_protocol = nullptr;
};

} // namespace bee