#pragma once
#include "session.h"
#include <openssl/ssl.h>

namespace bee
{

class httpprotocol;
class httpsession_manager;

class httpsession : public session
{
public:
    httpsession(httpsession_manager* manager);
    virtual ~httpsession();

    virtual void open() override;
    virtual void close() override;

    virtual void on_recv(size_t len) override;
    virtual void on_send(size_t len) override;

protected:
    void handle_request(httpprotocol* prot);

private:
    friend class httpsession_manager;
    SSL* _ssl = nullptr;
    size_t _request_count = 0;
    TIMETYPE _last_active = 0;
};

} // namespace bee