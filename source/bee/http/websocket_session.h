#pragma once
#include "session.h"
#include "httpprotocol.h"
#include <vector>
#include <openssl/sha.h>

namespace bee {

class websocket_session : public session
{
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
    
    void on_recv(size_t len) override
    {
        if(_state == HANDSHAKING)
        {
            handle_handshake();
        }
        else
        {
            process_websocket_frames();
        }
    }

    void send_message(opcode type, const octets& payload)
    {
        bee::rwlock::wrscoped l(_locker);
        frame_header header;
        header.fin = 1;
        header.opcode = static_cast<uint8_t>(type);
        header.mask = 0; // Server must not mask
        header.payload_len = payload.size();
        
        octetsstream os;
        pack_header(os, header);
        os.data().append(payload.data(), payload.size());
        _write_queue.push_back(os.data());
        permit_send();
    }

private:
    enum state { HANDSHAKING, CONNECTED, CLOSING };
    
    struct frame_header
    {
        uint8_t fin:1;
        uint8_t rsv:3;
        uint8_t opcode:4;
        uint8_t mask:1;
        uint64_t payload_len;
        uint32_t masking_key;
    };

    void handle_handshake()
    {
        // 解析HTTP头
        httpprotocol req;
        if (!req.unpack(_reados)) return;
        
        // 验证WebSocket头
        if (req.get_header("Upgrade") != "websocket" ||
            req.get_header("Connection").find("Upgrade") == std::string::npos) 
        {
            send_error_response(400, "Invalid WebSocket Request");
            return;
        }
        
        // 生成响应
        httpprotocol resp;
        resp.set_status(101, "Switching Protocols");
        resp.set_header("Upgrade", "websocket");
        resp.set_header("Connection", "Upgrade");
        resp.set_header("Sec-WebSocket-Accept", 
            generate_ws_accept(req.get_header("Sec-WebSocket-Key")));
        
        // 发送响应
        octetsstream os;
        resp.encode(os);
        _write_queue.push_back(os.data());
        permit_send();
        
        _state = CONNECTED;
    }

    void process_websocket_frames()
    {
        octets data(_reados.data());
        while(true) {
            frame_header header;
            if(!parse_frame(header, data)) break;
            
            if(header.payload_len > data.size()) break;
            
            octets payload(data.data(), header.payload_len);
            _reados.data().erase(0, header.payload_len);
            
            handle_frame(header, payload);
        }
    }

    bool parse_frame(frame_header& header, octets& payload)
    {
        const uint8_t* data = (const uint8_t*)_recv_buffer.data();
        size_t pos = 0;
        const size_t buffer_size = _recv_buffer.size();

        // 基本头校验
        if (buffer_size < 2) return false;
        
        // 解析操作码和标志位
        header.fin = (data[pos] >> 7) & 0x01;
        header.rsv = (data[pos] >> 4) & 0x07;
        header.opcode = data[pos++] & 0x0F;
        
        // RSV位必须为0
        if (header.rsv != 0) {
            send_close_frame(1002); // Protocol error
            return false;
        }

        // 解析长度字段
        header.mask = (data[pos] >> 7) & 0x01;
        uint64_t len = data[pos++] & 0x7F;
        
        if (len == 126) {
            if (buffer_size < pos + 2) return false;
            len = ntohs(*(uint16_t*)&data[pos]);
            pos += 2;
        } else if (len == 127) {
            if (buffer_size < pos + 8) return false;
            len = ntohll(*(uint64_t*)&data[pos]);
            pos += 8;
        }

        // 校验最大帧长度
        constexpr size_t MAX_FRAME_SIZE = 1 << 24; // 16MB
        if (len > MAX_FRAME_SIZE) {
            send_close_frame(1009); // Message too big
            return false;
        }

        // 解析掩码
        if (header.mask) {
            if (buffer_size < pos + 4) return false;
            header.masking_key = *(uint32_t*)&data[pos];
            pos += 4;
        }

        // 检查数据完整性
        if (buffer_size < pos + len) return false;

        // 应用掩码
        payload.assign(data + pos, len);
        if (header.mask) {
            uint8_t* p = payload.data();
            uint32_t key = header.masking_key;
            for (size_t i = 0; i < len; ++i) {
                p[i] ^= ((uint8_t*)(&key))[i % 4];
            }
        }

        _recv_buffer.erase(0, pos + len);
        return true;
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

    void pack_header(octetsstream& os, const frame_header& header)
    {
        os << static_cast<uint8_t>((header.fin << 7) | (header.rsv << 4) | header.opcode);
        if (header.payload_len <= 125) {
            os << static_cast<uint8_t>(header.payload_len);
        } else if (header.payload_len <= 65535) {
            os << static_cast<uint8_t>(126);
            os << htons(static_cast<uint16_t>(header.payload_len));
        } else {
            os << static_cast<uint8_t>(127);
            os << htonll(header.payload_len);
        }
    }

    void send_close_frame()
    {
        frame_header header;
        header.fin = 1;
        header.opcode = static_cast<uint8_t>(opcode::CLOSE);
        header.mask = 0;
        header.payload_len = 0;
        octetsstream os;
        pack_header(os, header);
        _write_queue.push_back(os.data());
        permit_send();
    }

    void send_pong_frame(const octets& payload) {
        frame_header header;
        header.fin = 1;
        header.opcode = static_cast<uint8_t>(opcode::PONG);
        header.mask = 0;
        header.payload_len = payload.size();
        octetsstream os;
        pack_header(os, header);
        os.data().append(payload.data(), payload.size());
        _write_queue.push_back(os.data());
        permit_send();
    }

    std::string base64_encode(const unsigned char* input, size_t length) {
        static const char encode_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        encoded.reserve(((length + 2) / 3) * 4);
        for (size_t i = 0; i < length; i += 3) {
            uint32_t triple = (input[i] << 16);
            if (i + 1 < length) triple |= (input[i + 1] << 8);
            if (i + 2 < length) triple |= input[i + 2];
            encoded.push_back(encode_table[(triple >> 18) & 0x3F]);
            encoded.push_back(encode_table[(triple >> 12) & 0x3F]);
            encoded.push_back((i + 1 < length) ? encode_table[(triple >> 6) & 0x3F] : '=');
            encoded.push_back((i + 2 < length) ? encode_table[triple & 0x3F] : '=');
        }
        return encoded;
    }
};
} // namespace bee 