#ifndef WEBSOCKET_SESSION_H
#define WEBSOCKET_SESSION_H

#include <string>

class WebSocketSession {
public:
    WebSocketSession();
    ~WebSocketSession();

    void sendMessage(const std::string& message);
    std::string receiveMessage();

private:
    // WebSocket连接相关的私有成员
};

#endif // WEBSOCKET_SESSION_H 