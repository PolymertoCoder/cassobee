#pragma once

#include "concept.h"
#include "octets.h"
#include "types.h"
#include "bytes_order.h"

namespace bee
{

class octetsstream;

class marshal
{
public:
    virtual octetsstream& pack(octetsstream& os) const = 0;
    virtual octetsstream& unpack(octetsstream& os) = 0;
};

class octetsstream
{
public:
    CLASS_EXCEPTION_DEFINE(exception);
    enum Transaction : uint8_t
    {
        BEGIN,
        ROLLBACK,
        COMMIT
    };

public:
    octetsstream() : _data() {}
    octetsstream(const octets& oct) : _data(oct) {}
    octetsstream(const octets& oct, size_t size) : _data(oct, size) {}

    template<typename T> FORCE_INLINE octetsstream& operator <<(const T& val) { return push(val); }
    template<typename T> FORCE_INLINE octetsstream& operator >>(T&& val) { return pop(std::forward<T>(val)); }

    FORCE_INLINE void swap(octetsstream& rhs)
    {
        _data.swap(rhs._data);
        std::swap(_pos, rhs._pos);
        std::swap(_transpos, rhs._transpos);
    }
    FORCE_INLINE void try_shrink()
    {
        if(_pos > 0x100000)
        {
            _data.erase(0, _pos);
            _pos = 0;
            _transpos = 0;
        }
    }
    FORCE_INLINE void reserve(size_t cap) { _data.reserve(cap); }
    FORCE_INLINE octets& data() { return _data; }
    FORCE_INLINE void advance(size_t len) { _pos += len; }
    FORCE_INLINE void clear() { _data.clear(); _pos = 0; _transpos = 0; }
    FORCE_INLINE bool data_ready(size_t len) const { return (_data.size() - _pos) >= len; }
    FORCE_INLINE size_t get_pos() const { return _pos; }
    FORCE_INLINE size_t size() const { return _data.size(); }
    FORCE_INLINE bool empty() const { return _data.empty(); }
    FORCE_INLINE size_t capacity() const { return _data.capacity(); }

public:
    // 标准布局类型 push & pop
    template<bee::trivially_copyable T> octetsstream& push(const T& val)
    {
        T temp = hostToNetwork(val);
        _data.append((char*)&temp, sizeof(T));
        return *this;
    }

    template<bee::trivially_copyable T> octetsstream& pop(T& val)
    {
        size_t len = sizeof(val);
        if(_pos + len > _data.size())
        {
            throw exception("no enough data!!!");
        }
        memcpy(&val, _data.begin() + _pos, len);
        if constexpr(sizeof(T) > 1)
        {
            val = networkToHost(val);
        }
        _pos += len;
        return *this;
    }

    octetsstream& push(const octets& val)
    {
        push(val.size());
        _data.append(val.data(), val.size());
        return *this;
    }

    octetsstream& pop(octets& val)
    {
        size_t size = 0;
        pop(size);
        if(_pos + size > _data.size())
        {
            throw exception("no enougn data!!!");
        }
        val.append(_data.begin() + _pos, size);
        _pos += size;
        return *this;
    }

    octetsstream& push(Transaction val) = delete;

    octetsstream& pop(Transaction val)
    {
        switch(val)
        {
            case BEGIN: { _transpos = _pos; } break;
            case ROLLBACK:
            {
                if(_transpos > _data.size())
                {
                    throw exception("invalid transaction rollback position");
                }
                _pos = _transpos;
            } break;
            case COMMIT: { try_shrink(); } break;
            default: { throw exception("Invalid transaction operation"); } 
        }
        return *this;
    }

    octetsstream& push(const marshal& val)
    {
        return val.pack(*this);
    }

    octetsstream& pop(marshal& val)
    {
        return val.unpack(*this);
    }

    template<typename T, typename U> octetsstream& push(const std::pair<T, U>& val)
    {
        push(val.first);
        push(val.second);
        return *this;
    }

    template<typename T, typename U> octetsstream& pop(std::pair<T, U>& val)
    {
        pop(val.first);
        pop(val.second);
        return *this;
    }

    template<std::same_as<char> T> octetsstream& push(const std::basic_string<T>& val)
    {
        push(val.size());
        _data.append(val.data(), val.size());
        //printf("push %s size:%zu\n", val.data(), val.size());
        return *this;
    }

    template<std::same_as<char> T> octetsstream& pop(std::basic_string<T>& val)
    {
        size_t size = 0;
        pop(size);
        if(_pos + size > _data.size())
        {
            throw exception("no enough data!!!");
        }
        val.append(_data.begin() + _pos, size);
        _pos += size;
        //printf("pop %s size:%zu\n", val.data(), size);
        return *this;
    }

    // STL容器 push & pop
    template<bee::stl_container T> octetsstream& push(const T& val)
    {
        return push_container(val);
    }

    template<bee::stl_container T> octetsstream& pop(T& val)
    {
        return pop_container(val);
    }

protected:
    template<bee::stl_container container_type>
    octetsstream& push_container(const container_type& container)
    {
        push(container.size());
        for(auto iter = container.cbegin(); iter != container.cend(); ++iter)
        {
            push(*iter);
        }
        return *this;
    }

    template<bee::can_reserve_stl_container container_type>
    octetsstream& pop_container(container_type& container)
    {
        using size_type  = container_type::size_type;
        using value_type = container_type::value_type;
        size_type size = 0;
        pop(size);
        container.reserve(size);
        for(size_t i = 0; i < size; ++i)
        {
            value_type element;
            pop(element);
            container.insert(container.end(), element);
        }
        return *this;
    }

    template<typename container_type>
    octetsstream& pop_container(container_type& container)
    {
        using size_type  = typename container_type::size_type;
        using value_type = container_type::value_type;
        size_type size = 0;
        pop(size);
        for(size_t i = 0; i < size; ++i)
        {
            value_type element;
            pop(element);
            container.insert(container.end(), element);
        }
        return *this;
    }

    template<typename T, typename U>
    octetsstream& pop_container(std::map<T, U>& container)
    {
        size_t size = 0;
        pop(size);
        for(size_t i = 0; i < size; ++i)
        {
            std::pair<T, U> element;
            pop(element);
            container.insert(element);
        }
        return *this;
    }

    template<typename T, typename U>
    octetsstream& pop_container(std::unordered_map<T, U>& container)
    {
        size_t size = 0;
        pop(size);
        for(size_t i = 0; i < size; ++i)
        {
            std::pair<T, U> element;
            pop(element);
            container.insert(element);
        }
        return *this;
    }

private:
    octets _data;
    size_t _pos = 0;
    size_t _transpos = 0;
};

} // namespace bee