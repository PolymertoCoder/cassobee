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
    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    static rpc* call(PROTOCOLID id, const rpcdata& argument);

    virtual bool server(rpcdata* argument, rpcdata* result) = 0; // 返回false表示继续投递
    virtual void client(rpcdata* argument, rpcdata* result) {}
    virtual void timeout(rpcdata* argument);

public:
    TRACEID _traceid = 0;
    bool _is_server = false;
    bool _is_proxy = false; // 是否是中转的rpc
    rpcdata* _argument = nullptr;
    rpcdata* _result = nullptr;

protected:
    static void add_rpc_cache(rpc* prpc, bool is_proxy = false);
    bool do_server();
    void do_client();
    void do_timeout();

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
        _timeout_hdl ? _timeout_hdl(argument) : timeout(argument);
    }

    static rpc* call(const rpcdata& argument, rpc_callback::client_handler client_hdl, rpc_callback::timeout_handler timeout_hdl = {})
    {
        // rpc* prpc = (rpc*)get_protocol(id);
        // if(!prpc) return nullptr;
        // prpc->_is_server = false;
        // prpc->_argument = argument.dup();
        // add_rpc_cache(prpc);
        // return prpc;
        // rpc* prpc = rpc::call(RPC::get_type(), argument);
        // if(!prpc) return nullptr;
        // _client_hdl = client_hdl;
        // _timeout_hdl = timeout_hdl;
    }

protected:
    client_handler  _client_hdl;
    timeout_handler _timeout_hdl;
};

} // namespace bee