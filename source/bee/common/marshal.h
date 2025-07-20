#pragma once

#include "concept.h"
#include "octets.h"
#include "types.h"
#include "bytes_order.h"
#include "varint.h"

namespace bee
{

class octetsstream;

class marshal
{
public:
    virtual octetsstream& pack(octetsstream& os) const = 0;
    virtual octetsstream& unpack(octetsstream& os) = 0;
};

template<std::integral T>
struct compact_int : public marshal
{
private:
    T value = 0;

public:
    compact_int() = default;
    compact_int(T val) : value(val) {}
    inline operator T() const { return value; }
    inline operator T&() { return value; }
    inline operator const T&() const { return value; }
    inline compact_int& operator=(T val) { value = val; return *this; }
    inline T get() const { return value; }

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;
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
    octetsstream() = default;
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

    // POD 类型
    template<bee::pod T> FORCE_INLINE octetsstream& push(const T& val)
    {
        if constexpr(std::is_integral_v<T> && sizeof(T) > 1)
        {
            return push(compact_int(val));
        }

        T temp = hostToNetwork(val);
        _data.append(reinterpret_cast<const char*>(&temp), sizeof(T));
        return *this;
    }

    template<bee::pod T> FORCE_INLINE octetsstream& pop(T& val)
    {
        if constexpr(std::is_integral_v<T> && sizeof(T) > 1)
        {
            compact_int<T> compact;
            pop(compact);
            val = compact;
            return *this;
        }

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

    // octets
    FORCE_INLINE octetsstream& push(const octets& val)
    {
        push(val.size());
        _data.append(val.data(), val.size());
        return *this;
    }

    FORCE_INLINE octetsstream& pop(octets& val)
    {
        size_t size = 0;
        pop(size);
        if(_pos + size > _data.size())
        {
            throw exception("no enough data!!!");
        }
        val.append(_data.begin() + _pos, size);
        _pos += size;
        return *this;
    }

    // Transaction
    octetsstream& push(Transaction val) = delete;

    FORCE_INLINE octetsstream& pop(Transaction val)
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

    // marshal
    FORCE_INLINE octetsstream& push(const marshal& val)
    {
        return val.pack(*this);
    }

    FORCE_INLINE octetsstream& pop(marshal& val)
    {
        return val.unpack(*this);
    }

    // std::pair
    template<typename T, typename U> FORCE_INLINE octetsstream& push(const std::pair<T, U>& val)
    {
        push(val.first);
        push(val.second);
        return *this;
    }

    template<typename T, typename U> FORCE_INLINE octetsstream& pop(std::pair<T, U>& val)
    {
        pop(val.first);
        pop(val.second);
        return *this;
    }

    // STL容器的通用接口（not_pod排除了元素类型为pod的std::array容器）
    template<bee::not_pod_stl_container T> FORCE_INLINE octetsstream& push(const T& val)
    {
        return push_container(val);
    }

    template<bee::not_pod_stl_container T> FORCE_INLINE octetsstream& pop(T& val)
    {
        return pop_container(val);
    }

protected:
    // STL容器序列化接口
    template<bee::not_pod_stl_container Container>
    FORCE_INLINE octetsstream& push_container(const Container& container)
    {
        using value_type = typename Container::value_type;
        size_t size = container.size();

        if constexpr(!bee::is_std_array<Container>::value)
        {
            push(compact_int(size));
        }

        if constexpr(bee::continous_stl_container<Container> && bee::pod<value_type>)
        {
            size_t total_bytes = size * sizeof(value_type);
            _data.append(reinterpret_cast<const char*>(container.data()), total_bytes);
        }
        else
        {
            for(const auto& element : container)
            {
                push(element);
            }
        }
        return *this;
    }

    template<bee::not_pod_stl_container Container>
    FORCE_INLINE octetsstream& pop_container(Container& container)
    {
        using value_type = typename Container::value_type;
        using size_type  = typename Container::size_type;
        compact_int<size_type> size;

        if constexpr(bee::is_std_array<Container>::value)
        {
            size = container.size();
        }
        else
        {
            pop(size);
            if constexpr(bee::can_reserve_stl_container<Container>)
            {
                container.reserve(size);
            }
        }

        if constexpr(bee::continous_stl_container<Container> && bee::pod<value_type>)
        {
            size_t total_bytes = size * sizeof(value_type);
            if(_pos + total_bytes > _data.size())
            {
                throw exception("no enough data!!!");
            }

            if constexpr(is_std_array<Container>::value)
            {
                memcpy(container.data(), _data.begin() + _pos, total_bytes);
            }
            else
            {
                container.resize(size);
                memcpy(container.data(), _data.begin() + _pos, total_bytes);
            }
            _pos += total_bytes;
        }
        else if constexpr(is_std_map_v<Container> || is_std_multimap_v<Container> ||
                          is_std_unordered_map_v<Container> || is_std_unordered_multimap_v<Container>) // map
        {
            for(size_t i = 0; i < size; ++i)
            {
                std::pair<typename Container::key_type, typename Container::mapped_type> kv;
                pop(kv);
                container.insert(std::move(kv));
            }
        }
        else
        {
            for(size_t i = 0; i < size; ++i)
            {
                value_type element;
                pop(element);
                container.insert(container.end(), std::move(element));
            }
        }
        return *this;
    }

public:
    // compact int 接口
    FORCE_INLINE octetsstream& push_compact16(int16_t value)
    {
        return push_compactu16(bee::encode_zigzag16(value));
    }

    FORCE_INLINE octetsstream& pop_compact16(int16_t& value)
    {
        uint16_t tmp = 0;
        pop_compactu16(tmp);
        value = bee::decode_zigzag16(tmp);
        return *this;
    }

    FORCE_INLINE octetsstream& push_compactu16(uint16_t value)
    {
        return push_compact(static_cast<uint64_t>(value));
    }

    FORCE_INLINE octetsstream& pop_compactu16(uint16_t& value)
    {
        uint64_t tmp = 0;
        pop_compact(tmp);
        value = static_cast<uint16_t>(tmp);
        return *this;
    }

    FORCE_INLINE octetsstream& push_compact32(int32_t value)
    {
        return push_compactu32(bee::encode_zigzag32(value));
    }

    FORCE_INLINE octetsstream& pop_compact32(int32_t& value)
    {
        uint32_t temp = 0;
        pop_compactu32(temp);
        value = bee::decode_zigzag32(temp);
        return *this;
    }

    FORCE_INLINE octetsstream& push_compactu32(uint32_t value)
    {
        return push_compact(static_cast<uint64_t>(value));
    }

    FORCE_INLINE octetsstream& pop_compactu32(uint32_t& value)
    {
        uint64_t tmp = 0;
        pop_compact(tmp);
        value = static_cast<uint32_t>(tmp);
        return *this;
    }

    FORCE_INLINE octetsstream& push_compact64(int64_t value)
    {
        return push_compactu64(bee::encode_zigzag64(value));
    }

    FORCE_INLINE octetsstream& pop_compact64(int64_t& value)
    {
        uint64_t temp = 0;
        pop_compactu64(temp);
        value = bee::decode_zigzag64(temp);
        return *this;
    }

    FORCE_INLINE octetsstream& push_compactu64(uint64_t value)
    {
        return push_compact(value);
    }

    FORCE_INLINE octetsstream& pop_compactu64(uint64_t& value)
    {
        return pop_compact(value);
    }

protected:
    // varint编码本身实现了逻辑上“小端字节序”，因此不用考虑网络字节序
    FORCE_INLINE octetsstream& push_compact(uint64_t value)
    {
        thread_local std::vector<char> buffer;
        buffer.clear();
        bee::write_varint(value, buffer);
        _data.append(buffer.data(), buffer.size());
        return *this;
    }

    FORCE_INLINE octetsstream& pop_compact(uint64_t& value)
    {
        const char* ptr = _data.begin() + _pos;
        const char* end = _data.end();
        const char* new_ptr = bee::read_varint(ptr, end, value);
        if(new_ptr == nullptr)
        {
            throw exception("varint decode failed");
        }
        _pos += new_ptr - ptr;
        return *this;
    }

private:
    octets _data;
    size_t _pos = 0;
    size_t _transpos = 0;
};

template<std::integral T>
inline octetsstream& compact_int<T>::pack(octetsstream& os) const
{
    if constexpr(std::is_signed_v<T>)
    {
        if constexpr(sizeof(T) == 1)
            return os.push(static_cast<int8_t>(value));
        else if constexpr(sizeof(T) == 2)
            return os.push_compact16(static_cast<int32_t>(value));
        else if constexpr(sizeof(T) == 4)
            return os.push_compact32(static_cast<int32_t>(value));
        else if constexpr(sizeof(T) == 8)
            return os.push_compact64(static_cast<int64_t>(value));
        else static_assert(false, "compact_int<T>::pack:unsupported type");
    }
    else
    {
        if constexpr(sizeof(T) == 1)
            return os.push(static_cast<uint8_t>(value));
        else if constexpr(sizeof(T) == 2)
            return os.push_compactu16(static_cast<int32_t>(value));
        else if constexpr(sizeof(T) == 4)
            return os.push_compactu32(static_cast<uint32_t>(value));
        else if constexpr(sizeof(T) == 8)
            return os.push_compactu64(static_cast<uint64_t>(value));
        else static_assert(false, "compact_int<T>::pack:unsupported type");
    }
    return os;
}

template<std::integral T>
inline octetsstream& compact_int<T>::unpack(octetsstream& os)
{
    if constexpr(std::is_signed_v<T>) {
        if constexpr(sizeof(T) == 1) {
            os.pop(value);
        } else if constexpr(sizeof(T) == 2) {
            os.pop_compact16(value);
        } else if constexpr(sizeof(T) == 4) {
            os.pop_compact32(value);
        } else if constexpr(sizeof(T) == 8) {
            os.pop_compact64(value);
        } else {
            static_assert(false, "compact_int<T>::unpack:unsupported type");
        }
    } else {
        if constexpr(sizeof(T) == 1) {
            os.pop(value);
        } else if constexpr(sizeof(T) == 2) {
            os.pop_compactu16(value);
        } else if constexpr(sizeof(T) == 4) {
            os.pop_compactu32(value);
        } else if constexpr(sizeof(T) == 8) {
            os.pop_compactu64(value);
        } else {
            static_assert(false, "compact_int<T>::unpack unsupported type");
        }
    }
    return os;
}

template<std::integral T>
inline void encode_compact_int(T value, std::vector<char>& buffer)
{
    write_varint(value, buffer);
}

template<std::integral T>
inline void decode_compact_int(const char* ptr, const char* end, uint64_t& value)
{
    read_varint(ptr, end, value);
}

} // namespace bee