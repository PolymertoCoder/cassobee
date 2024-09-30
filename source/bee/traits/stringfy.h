#pragma once
#include "octets.h"
#include "traits.h"
#include "concept.h"
#include <utility>
#include <string>

namespace cassobee
{
    // forward declaration
    std::string to_string(const char* val);
    std::string to_string(const octets& val);
    std::string to_string(const std::string& val);
    std::string to_string(const std::string_view& val);

    template<typename T> requires std::is_arithmetic_v<T> std::string to_string(T val);
    template<typename T1, typename T2> std::string to_string(const std::pair<T1, T2>& val);
    template<typename T> requires cassobee::stl_container<T> std::string to_string(const T& val, bool prefix = false);
    template<typename... Args> std::string to_string(const std::tuple<Args...>& val);
}

namespace cassobee
{
inline std::string to_string(const char* val) { return val; }
inline std::string to_string(const octets& val) { return val; }
inline std::string to_string(const std::string& val) { return val; }
inline std::string to_string(const std::string_view& val) { return {val.data(), val.size()}; }

template<typename T>
requires std::is_arithmetic_v<T>
std::string to_string(T val) { return std::to_string(val); }

template<typename T1, typename T2>
std::string to_string(const std::pair<T1, T2>& val)
{
    std::string str("<");
    str.append(to_string(val.first) + ", ");
    str.append(to_string(val.second) + ">");
    return str;
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
} // namespace cassobee::detail

template<typename T>
requires cassobee::stl_container<T>
std::string to_string(const T& val, bool prefix)
{
    return detail::container_to_string(val, prefix ? cassobee::type_name<T>() : "");
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

} // namespace cassobee