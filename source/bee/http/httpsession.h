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
    void update_last_active(); // 更新会话的最后活跃时间
    bool is_timeout(TIMETYPE timeout) const; // 检查会话是否超时

private:
    friend class httpsession_manager;
    SSL* _ssl = nullptr;
    size_t _request_count = 0;
    TIMETYPE _last_active = 0; // 最后活跃时间
};

} // namespace bee