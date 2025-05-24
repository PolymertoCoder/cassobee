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
    _is_server = other._is_server;
    if(other._argument)
    {
        _argument = other._argument->dup();
    }
    if(other._result)
    {
        _result = other._result->dup();
    }
}

rpc::rpc(rpc&& other)
    : protocol(other)
{
    _traceid = other._traceid;
    _is_server = other._is_server;
    if(other._argument)
    {
        _argument = other._argument;
        other._argument = nullptr;
    }
    if(other._result)
    {
        _result = other._result;
        other._result = nullptr;
    }
}

rpc& rpc::operator=(const rpc& rhs)
{
    if(this != &rhs)
    {
        protocol::operator=(rhs);
        _traceid = rhs._traceid;
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
            this->_is_server = false; // 转换身份
            this->_is_proxy = true; // 标记为中转的rpc
            add_rpc_cache(this, true);
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
            if(prpc->_is_proxy) // 是中转的rpc，开始回溯寻找调用方client
            {
                prpc->_is_server = true; // 回溯时，转换回原身份
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

octetsstream& rpc::pack(octetsstream& os) const
{
    os << _traceid;
    os << !_is_server; // 发出时需要转换身份
    os << *_argument;
    if(_is_server) // server
    {
        os << *_result;
    }
    return os;
}

octetsstream& rpc::unpack(octetsstream& os)
{
    os >> _traceid;
    os >> _is_server;
    os >> *_argument;
    if(!_is_server) // client
    {
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
    prpc->_is_server = false;
    prpc->_argument = argument.dup();
    add_rpc_cache(prpc);
    return prpc;
}

void rpc::add_rpc_cache(rpc* prpc, bool is_proxy)
{
    if(!prpc) return;
    bee::mutex::scoped l(_locker);
    static TRACEID next_traceid = 0;
    ++next_traceid;

    // 如果不是中转暂存的，则需要分配一个新的traceid
    // 如果是中转的rpc，则需要保留原来的traceid，方便回溯
    if(!is_proxy)
    {
        prpc->_traceid = next_traceid;
    }
    _rpcs.emplace(next_traceid, prpc);

    // 设置超时定时器
    add_timer(prpc->get_timeout() * 1000, [traceid = next_traceid]()
    {
        rpc* prpc = nullptr;
        {
            if(auto iter = _rpcs.find(traceid); iter != _rpcs.end())
            {
                prpc = iter->second;
                _rpcs.erase(iter);
            }
        }
        prpc->do_timeout();
        delete prpc;
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