#pragma once
#include <type_traits>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace cassobee
{

template<typename container_type>
struct is_stl_container : std::false_type {};

template<typename T>
struct is_stl_container<std::vector<T>> : std::true_type {};

template<typename T>
struct is_stl_container<std::set<T>> : std::true_type {};

template<typename T>
struct is_stl_container<std::unordered_set<T>> : std::true_type {};

template<typename T, typename U>
struct is_stl_container<std::map<T, U>> : std::true_type {};

template<typename T, typename U>
struct is_stl_container<std::unordered_map<T, U>> : std::true_type {};

template<typename container_type>
constexpr bool is_stl_container_v = is_stl_container<container_type>::value;

}