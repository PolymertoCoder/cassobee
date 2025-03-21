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

    virtual httpsession* dup() override;

    virtual void open() override;
    virtual void close() override;

    virtual void on_recv(size_t len) override;
    virtual void on_send(size_t len) override;

    FORCE_INLINE SSL* get_ssl() const { return _ssl; }

private:
    friend class httpsession_manager;
    SSL* _ssl = nullptr;
};

} // namespace bee