#ifndef HTTPSESSION_MANAGER_H
#define HTTPSESSION_MANAGER_H

#include <vector>
#include "httpsession.h"

class HttpSessionManager {
public:
    HttpSessionManager();
    ~HttpSessionManager();

    void addSession(HttpSession* session);
    void removeSession(HttpSession* session);
    void manageSessions();

private:
    std::vector<HttpSession*> sessions;
};

#endif // HTTPSESSION_MANAGER_H 