#pragma once

#include <deque>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace cassobee
{

template<typename T> std::string to_string(const std::vector<T>& val);
template<typename T, size_t N> std::string to_string(const std::array<T, N>& val);
template<typename T> std::string to_string(const std::deque<T>& val);
template<typename T> std::string to_string(const std::set<T>& val);
template<typename T> std::string to_string(const std::unordered_set<T>& val);
template<typename T1, typename T2> std::string to_string(const std::map<T1, T2>& val);
template<typename T1, typename T2> std::string to_string(const std::unordered_map<T1, T2>& val);

template<typename T>
auto to_string(T val) -> typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
{
    return std::to_string(val);
}

template<typename T1, typename T2>
std::string to_string(const std::pair<T1, T2>& val)
{
    std::string str("<");
    str.append(to_string(val.first) + ", ");
    str.append(to_string(val.second) + ">");
    return str;
}

template<typename STL_CONTAINER>
std::string container_to_string(const STL_CONTAINER& container, const std::string_view& prefix)
{
    std::string str(prefix.data(), prefix.size());
    size_t i = 0, sz = container.size();
    for(typename STL_CONTAINER::const_iterator iter = container.begin(); iter != container.end(); ++iter, ++i)
    {
        str.append(ToString(*iter));
        if(i != sz-1){ str.append(", "); }
    }
    return str.append("}");
}

template<typename T>
std::string to_string(const std::vector<T>& val)
{
    return container_to_string(val, "std::vector:{");
}
template<typename T, size_t N>
std::string to_string(const std::array<T, N>& val)
{
    return container_to_string(val, "std::array:{");
}
template<typename T>
std::string to_string(const std::deque<T>& val)
{
    return container_to_string(val, "std::deque:{");
}
template<typename T>
std::string to_string(const std::set<T>& val)
{
    return container_to_string(val, "std::set:{");
}
template<typename T>
std::string to_string(const std::unordered_set<T>& val)
{
    return container_to_string(val, "std::unordered_set:{");
}
template<typename T1, typename T2>
std::string to_string(const std::map<T1, T2>& val)
{
    return container_to_string(val, "std::map:{");
}
template<typename T1, typename T2>
std::string to_string(const std::unordered_map<T1, T2>& val)
{
    return container_to_string(val, "std::unordered_map:{");
}


}