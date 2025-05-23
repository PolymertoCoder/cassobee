#pragma once
#include "common.h"
#include "octets.h"
#include "traits.h"
#include "concept.h"

namespace bee
{
    // forward declaration
    std::string to_string(const char* val);
    std::string to_string(const octets& val);
    std::string to_string(const std::string& val);
    std::string to_string(const std::string_view& val);

    template<bee::arithmetic T> std::string to_string(T val);
    template<typename T> requires std::is_enum_v<T> std::string to_string(T val);
    template<typename T1, typename T2> std::string to_string(const std::pair<T1, T2>& val);
    template<stl_container T> std::string to_string(const T& val, bool prefix = false);
    template<typename... Args> std::string to_string(const std::tuple<Args...>& val);
}

namespace bee
{
inline std::string to_string(const char* val) { return val; }
inline std::string to_string(const octets& val) { return val; }
inline std::string to_string(const std::string& val) { return val; }
inline std::string to_string(const std::string_view& val) { return {val.data(), val.size()}; }

template<bee::arithmetic T>
std::string to_string(T val) { return std::to_string(val); }

template<typename T> requires std::is_enum_v<T> std::string to_string(T val)
{
    return to_string(static_cast<std::underlying_type_t<T>>(val));
}

template<typename T1, typename T2>
std::string to_string(const std::pair<T1, T2>& val)
{
    return format_string("<%s, %s>", to_string(val.first).data(), to_string(val.second).data());
}

namespace detail
{
template<typename STL_CONTAINER>
std::string container_to_string(const STL_CONTAINER& container, const std::string_view& prefix)
{
    std::string str(prefix.data(), prefix.size());
    str.append("{");
    size_t i = 0, sz = container.size();
    for(typename STL_CONTAINER::const_iterator iter = container.begin(); iter != container.end(); ++iter, ++i)
    {
        str.append(to_string(*iter));
        if(i != sz - 1){ str.append(", "); }
    }
    return str.append("}");
}
} // namespace bee::detail

template<stl_container T>
std::string to_string(const T& val, bool prefix)
{
    return detail::container_to_string(val, prefix ? bee::type_name<T>() : "");
}

template<typename... Args>
std::string to_string(const std::tuple<Args...>& val)
{
    constexpr size_t tuple_size = sizeof...(Args);
    std::string str("{");
    [&]<size_t... Is>(std::index_sequence<Is...>&&)
    {
        str += ((to_string(std::get<Is>(val)) + ((Is != tuple_size - 1) ? " " : "")) + ...);
    }(std::make_index_sequence<tuple_size>());
    return str.append("}");
}

template<typename T>
concept can_to_string = requires(T t) { { bee::to_string(t) } -> std::convertible_to<std::string>; };

} // namespace bee