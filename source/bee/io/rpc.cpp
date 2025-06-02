#include "rpc.h"
#include "glog.h"
#include "protocol.h"
#include "reactor.h"
#include "session_manager.h"

namespace bee
{

bee::mutex rpc::_locker;
std::map<TRACEID, rpc*> rpc::_rpcs;

rpc::rpc(const rpc& other)
    : protocol(other)
{
    _traceid = other._traceid;
    _proxy_traceid = other._proxy_traceid;
    _is_server = other._is_server;
    _argument = other._argument; // 浅拷贝
    _result = other._result; // 浅拷贝
}

rpc::rpc(rpc&& other)
    : protocol(other)
{
    _traceid = other._traceid;
    _proxy_traceid = other._proxy_traceid;
    _is_server = other._is_server;
    std::swap(_argument, other._argument);
    std::swap(_result, other._result);
}

rpc& rpc::operator=(const rpc& rhs)
{
    if(this != &rhs)
    {
        protocol::operator=(rhs);
        _traceid = rhs._traceid;
        _proxy_traceid = rhs._proxy_traceid;
        _is_server = rhs._is_server;
        if(_argument) delete _argument;
        if(_result) delete _result;
        _argument = rhs._argument ? rhs._argument->dup() : nullptr;
        _result = rhs._result ? rhs._result->dup() : nullptr;
    }
    return *this;
}

rpc::~rpc()
{
    delete _argument;
    delete _result;
}

void rpc::run()
{
    if(_is_server) // server
    {
        if(do_server())
        {
            _manager->send_protocol(_sid, *this);
        }
        else // 继续投递
        {
            set_request(this, true); // 标记为中转的rpc
        }
    }
    else // client
    {
        rpc* prpc = nullptr;
        {
            bee::mutex::scoped l(_locker);
            if(auto iter = _rpcs.find(_traceid); iter != _rpcs.end())
            {
                prpc = iter->second;
                _rpcs.erase(iter);
            }
        }
        if(prpc)
        {
            std::swap(prpc->_result, this->_result);
            if(prpc->_proxy_traceid > 0) // 是中转的rpc，开始回溯寻找调用方client
            {
                clr_request(prpc, true);
                prpc->_manager->send_protocol(prpc->_sid, *prpc);
                return;
            }
            else
            {
                prpc->do_client();
            }
            delete prpc;
        }
    }
}

ostringstream& rpc::dump(ostringstream& out) const
{
    out << "rpc " << get_type() << " traceid=" << _traceid << " is_server=" << _is_server;
    out << " result=";
    if(_result) {
        _result->dump(out);
    } else {
        out << "nullptr";
    }

    out << " argument=";
    if(_argument) {
        _argument->dump(out);
    } else {
        out << "nullptr";
    }
    return out;
}

octetsstream& rpc::pack(octetsstream& os) const
{
    os << _traceid << _is_server;
    if(_is_server) // server
    {
        os << *_result;
    }
    else // client
    {
        os << *_argument;
    }
    return os;
}

octetsstream& rpc::unpack(octetsstream& os)
{
    os >> _traceid >> _is_server;
    _is_server = !_is_server; // 身份反转
    if(_is_server) // server
    {
        _argument = _argument->dup();
        os >> *_argument;
        _result = _result->dup();
    }
    else // client
    {
        _argument = nullptr;
        _result = _result->dup();
        os >> *_result;
    }
    return os;
}

void rpc::timeout(rpcdata* argument)
{
    thread_local ostringstream oss;
    oss.clear();
    argument->dump(oss);
    local_log("rpc %u timeout, traceid=%lu, argument:%s.", get_type(), _traceid, oss.c_str());
}

rpc* rpc::call(PROTOCOLID id, const rpcdata& argument)
{
    rpc* prpc = (rpc*)get_protocol(id);
    if(!prpc) return nullptr;
    prpc->_argument = argument.dup();
    prpc->_result = nullptr;
    set_request(prpc);
    return prpc;
}

void rpc::set_request(rpc* prpc, bool is_proxy)
{
    if(!prpc) return;

    if(is_proxy) // 如果是中转的rpc
    {
        prpc->_proxy_traceid = prpc->_traceid; // 保留原来的traceid
    }

    {
        bee::mutex::scoped l(_locker);
        static TRACEID next_traceid = 0;
        prpc->_traceid = ++next_traceid;
        prpc->_is_server = false; // 设置client身份
        _rpcs.emplace(prpc->_traceid, prpc);
    }

    // 设置超时定时器
    TIMETYPE timeout = prpc->get_timeout();
    set_timeout_timer(timeout > 0 ? timeout : 30, prpc->_traceid);
}

void rpc::clr_request(rpc* prpc, bool is_proxy)
{
    if(!prpc) return;
    prpc->_is_server = true;
    if(is_proxy)
    {
        prpc->_traceid = 0;
        std::swap(prpc->_traceid, prpc->_proxy_traceid);
    }
}

void rpc::set_timeout_timer(TRACEID traceid, int timeout)
{
    add_timer(timeout * 1000, [traceid]()
    {
        rpc* prpc = nullptr;
        {
            bee::mutex::scoped l(_locker);
            if(auto iter = _rpcs.find(traceid); iter != _rpcs.end())
            {
                prpc = iter->second;
                _rpcs.erase(iter);
            }
        }
        if(prpc)
        {
            prpc->do_timeout();
            delete prpc;
        }
        local_log("rpc timeout timer run.");
        return false;
    });
}

bool rpc::do_server()
{
    return server(_argument, _result);
}

void rpc::do_client()
{
    client(_argument, _result);
}

void rpc::do_timeout()
{
    timeout(_argument);
}

} // namespace bee