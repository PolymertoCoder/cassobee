#pragma once
#include "protocol.h"
#include "types.h"
#include <concepts>

namespace bee
{

class rpc : public protocol
{
public:
    rpc() = default;
    rpc(PROTOCOLID type) : protocol(type) {}
    rpc(const rpc& other);
    rpc(rpc&& other);
    rpc& operator=(const rpc& rhs);
    virtual ~rpc() override;

    virtual int get_timeout() const { return 30; } // 默认30s超时
    virtual void run() override;
    virtual ostringstream& dump(ostringstream& out) const override;
    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    static rpc* call(PROTOCOLID id, const rpcdata& argument);
    static rpc* call(rpc* prpc);

    virtual bool server(rpcdata* argument, rpcdata* result) = 0; // 返回true表示继续投递
    virtual void client(rpcdata* argument, rpcdata* result) {}
    virtual void timeout(rpcdata* argument);

protected:
    static void set_request(rpc* prpc, bool is_proxy = false);
    static void clr_request(rpc* prpc, bool is_proxy = false);
    static void set_timeout_timer(TRACEID traceid, int timeout);
    bool do_server();
    void do_client();
    void do_timeout();

public:
    TRACEID _traceid = 0;
    TRACEID _proxy_traceid = 0; // 保留原来的traceid，方便回溯
    bool _is_server = false;
    rpcdata* _argument = nullptr;
    rpcdata* _result = nullptr;

protected:
    static bee::mutex _locker;
    static std::map<TRACEID, rpc*> _rpcs;
};

template<std::derived_from<rpc> RPC>
class rpc_callback : public RPC
{
public:
    using client_handler  = std::function<void(rpcdata* argument, rpcdata* result)>;
    using timeout_handler = std::function<void(rpcdata* argument)>;

    virtual void client(rpcdata* argument, rpcdata* result) override
    {
        if(_client_hdl) { _client_hdl(argument, result); }
    }

    virtual void timeout(rpcdata* argument) override
    {
        _timeout_hdl ? _timeout_hdl(argument) : RPC::timeout(argument);
    }

    static rpc* call(const RPC::argument_type& argument, client_handler client_hdl, timeout_handler timeout_hdl = {})
    {
        rpc_callback<RPC>* prpc_callback = new rpc_callback<RPC>;
        prpc_callback->_argument = argument.dup();
        prpc_callback->_result = nullptr;
        prpc_callback->_client_hdl = std::move(client_hdl);
        if(timeout_hdl) prpc_callback->_timeout_hdl = std::move(timeout_hdl);
        return rpc::call(prpc_callback);
    }

protected:
    client_handler  _client_hdl;
    timeout_handler _timeout_hdl;
};

} // namespace bee