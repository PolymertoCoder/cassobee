#pragma once
#include "session.h"
#include "http_protocol.h"
#include <array>
#include <print>

namespace bee {

class websocket_session : public session {
public:
    enum class opcode : uint8_t {
        CONTINUATION = 0x0,
        TEXT         = 0x1,
        BINARY       = 0x2,
        CLOSE        = 0x8,
        PING         = 0x9,
        PONG         = 0xA
    };

    websocket_session(session_manager* manager) 
        : session(manager), _state(HANDSHAKING) {}
    
    void on_recv(size_t len) override {
        if(_state == HANDSHAKING) {
            handle_handshake();
        } else {
            process_websocket_frames();
        }
    }

    void send_message(opcode type, const octets& payload) {
        std::lock_guard<bee::spinlock> l(_write_lock);
        frame_header header;
        header.fin = 1;
        header.opcode = static_cast<uint8_t>(type);
        header.mask = 0; // Server must not mask
        header.payload_len = payload.size();
        
        octetsstream os;
        pack_header(os, header);
        os.append(payload.data(), payload.size());
        _write_queue.push_back(os.data());
        permit_send();
    }

private:
    enum state { HANDSHAKING, CONNECTED, CLOSING };
    
    struct frame_header {
        uint8_t fin:1;
        uint8_t rsv:3;
        uint8_t opcode:4;
        uint8_t mask:1;
        uint64_t payload_len;
        uint32_t masking_key;
    };

    void handle_handshake() {
        http_protocol req;
        if(req.unpack(_reados)) {
            if(req.get_header("Upgrade") == "websocket") {
                send_handshake_response(req);
                _state = CONNECTED;
                _reados.data().erase(0, req.raw_size());
            }
        }
    }

    void send_handshake_response(const http_protocol& req) {
        http_protocol res;
        res.set_response(101, "Switching Protocols");
        res.set_header("Upgrade", "websocket");
        res.set_header("Connection", "Upgrade");
        res.set_header("Sec-WebSocket-Accept", 
            compute_accept_key(req.get_header("Sec-WebSocket-Key")));
        
        octetsstream os;
        res.encode(os);
        _writeos.data().append(os.data(), os.size());
        permit_send();
    }

    std::string compute_accept_key(const std::string& client_key) {
        // 实现RFC6455的密钥计算
        const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string combined = client_key + magic;
        unsigned char sha1[20];
        SHA1(reinterpret_cast<const unsigned char*>(combined.data()), 
            combined.size(), sha1);
        return base64_encode(sha1, sizeof(sha1));
    }

    void process_websocket_frames() {
        while(true) {
            frame_header header;
            if(!parse_header(header)) break;
            
            if(header.payload_len > _reados.size()) break;
            
            octets payload = _reados.data().suboctets(0, header.payload_len);
            _reados.data().erase(0, header.payload_len);
            
            handle_frame(header, payload);
        }
    }

    bool parse_header(frame_header& header) {
        // 实现WebSocket帧头解析
        // ... 详细解析逻辑 ...
    }

    void handle_frame(const frame_header& header, const octets& payload) {
        switch(static_cast<opcode>(header.opcode)) {
            case opcode::TEXT:
            case opcode::BINARY:
                on_message(header, payload);
                break;
            case opcode::CLOSE:
                send_close_frame();
                close();
                break;
            case opcode::PING:
                send_pong_frame(payload);
                break;
            // ... 其他opcode处理 ...
        }
    }

    virtual void on_message(const frame_header& header, const octets& payload) = 0;

    std::vector<octets> _write_queue;
    state _state;
    bee::spinlock _write_lock;
};
} // namespace bee