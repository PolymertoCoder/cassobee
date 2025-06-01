#include <map>
#include <list>
#include <assert.h>
#include <vector>
#include <functional>
#include <algorithm>
#include "glog.h"

template<typename lock_type>
class delegate_base;

template<typename lock_type>
class delegate_base
{
public:
    struct lock_guard
    {
        lock_guard(delegate_base<lock_type>* self)
            : _self(self)
        {
            _self->lock();
        }
        ~lock_guard()
        {
            _self->unlock();
        }
        delegate_base<lock_type>* _self;
    };
    void lock()
    {
        _locker.lock();
    }
    void unlock()
    {
        _locker.unlock();
    }

protected:
    lock_type _locker;
};

template<>
class delegate_base<void>
{
public:
    struct lock_guard {};
    void lock() {}
    void unlock() {}
};

template<typename return_type, typename lock_type, typename ...param_types>
class delegate : public delegate_base<lock_type>
{
public:
    using base = delegate_base<lock_type>;
    using lock_guard = typename base::lock_guard;
    using base::lock;
    using base::unlock;

    bool bind(std::function<return_type(param_types...)>&& executor)
    {
        lock_guard l(this);
        if(_executor) return false;
        _executor = std::move(executor);
        return true;
    }
    void force_bind(std::function<return_type(param_types...)>&& executor)
    {
        lock_guard l(this);
        _executor = std::move(executor);
    }
    bool unbind()
    {
        lock_guard l(this);
        if(!_executor) return false;
        _executor = {};
        return true;
    }
    bool isbound()
    {
        lock_guard l(this);
        return _executor;
    }
    return_type execute(param_types... params)
    {
        lock();
        ASSERT(_executor);
        auto tmp_executor = _executor;
        unlock();
        return tmp_executor(params...);
    }
private:
    std::function<return_type(param_types...)> _executor;
};

template<typename container_type>
struct delegate_storage_base // container adapter
{
    using iterator = typename container_type::iterator;
    container_type& get_storage(){ return _data; }
    iterator begin(){ return _data.begin(); }
    iterator end(){ return _data.end(); }
    bool empty(){ return _data.empty(); }
    container_type _data;
};

template<typename key_type, typename value_type>
struct delegate_vector_storage : delegate_storage_base<std::vector<std::pair<key_type, value_type>>>
{
    using base = delegate_storage_base<std::vector<std::pair<key_type, value_type>>>;
    void add(const std::pair<key_type, value_type>& kv)
    {
        base::get_storage().push_back(kv);
    }
    void del(const key_type& key)
    {
        auto& storage = base::get_storage();
        std::erase_if(storage, [&key](const auto& kv){ return kv.first == key; });
    }
    value_type& get_value(typename base::iterator iter){ return (*iter).second; }
};

template<typename key_type, typename value_type>
struct delegate_list_storage : delegate_storage_base<std::list<std::pair<key_type, value_type>>>
{
    using base = delegate_storage_base<std::list<std::pair<key_type, value_type>>>;
    void add(const std::pair<key_type, value_type>& kv)
    {
        base::get_storage().push_back(kv);
    }
    void del(const key_type& key)
    {
        auto& storage = base::get_storage();
        std::erase_if(storage, [&key](const auto& kv){ return kv.first == key; });
    }
    value_type& get_value(typename base::iterator iter){ return (*iter).second; }
};

template<typename key_type, typename value_type>
struct delegate_map_storage : delegate_storage_base<std::map<key_type, value_type>>
{
    using base = delegate_storage_base<std::map<key_type, value_type>>;
    void add(const std::pair<key_type, value_type>& kv)
    {
        base::get_storage().insert(kv);
    }
    void del(const key_type& key)
    {
        base::get_storage().erase(key);
    }
    value_type& get_value(typename base::iterator iter){ return iter->second; }
};

template<typename id_type, template<typename keytype, typename value_type> class storage_type, typename lock_type, typename ...param_types>
class multicast_delegate : public delegate_base<lock_type>
                         , public storage_type<id_type, std::function<void(param_types...)>>
{
public:
    using base = delegate_base<lock_type>;
    using base_storage = storage_type<id_type, std::function<void(param_types...)>>;
    using lock_guard = typename base::lock_guard;

    int bind(std::function<void(param_types...)>&& executor)
    {
        lock_guard l(this);
        int id = _maxid++;
        base_storage::add(std::make_pair(id, executor));
        return id;
    }

    void unbind(id_type id)
    {
        lock_guard l(this);
        base_storage::del(id);
    }

    bool isbound()
    {
        lock_guard l(this);
        return base_storage::get_storage().empty();
    }

    void broadcast(param_types... params)
    {
        lock_guard l(this);
        auto storage = base_storage::get_storage();
        if(storage.empty()) return;
        for(auto iter = storage.begin(); iter != storage.end(); ++iter)
        {
            try
            {
                base_storage::get_value(iter)(params...);
            }
            catch(...)
            {
                ERRORLOG("multicast_delegate broadcast throw exception!!!");
            }
        }
    }
private:
    id_type _maxid = 0;
};

// 声明一个单播委托类型
#define SINGLE_DELEGATE_DECLEAR(name, return_type, ...) \
    using name = delegate<return_type, bee::mutex, __VA_ARGS__>;

// 声明一个多播委托类型
#define MULTICAST_DELEGATE_DECLEAR(name, ...) \
    using name = multicast_delegate<int, delegate_list_storage, bee::mutex, __VA_ARGS__>;

// 声明一个可能有大量的多播委托类型
#define NUMEROUS_MULTICAST_DELEGATE_DECLEAR(name, ...) \
    using name = multicast_delegate<int, delegate_map_storage, bee::mutex, __VA_ARGS__>;
