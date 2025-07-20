#pragma once
#include <type_traits>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace bee
{

// template_of
template<typename T> struct template_of : std::false_type
{
    using type = void;
};

template<template<typename...> class Template, typename... Args>
struct template_of<Template<Args...>> : std::true_type
{
    using type = Template<>;
};

template<typename T, typename U>
constexpr bool is_template_of = std::is_same_v<typename template_of<T>::type, U>;

// stl container traits
template<typename container_type>
struct is_stl_container : std::false_type {};

template<typename container_type>
constexpr bool is_stl_container_v = is_stl_container<container_type>::value;

#define BEE_DEFINE_STD_CONTAINER(TRAITS, TYPE) \
    template<typename T> \
    struct TRAITS : std::false_type {}; \
    template<typename... Args> \
    struct TRAITS<TYPE<Args...>> : std::true_type {}; /*NOLINT*/ \
    template<typename... Args> \
    struct is_stl_container<TYPE<Args...>> : std::true_type {}; /*NOLINT*/ \
    template<typename T> \
    constexpr bool TRAITS##_v = TRAITS<T>::value; /*NOLINT*/

BEE_DEFINE_STD_CONTAINER(is_std_vector, std::vector)
BEE_DEFINE_STD_CONTAINER(is_std_list, std::list)
BEE_DEFINE_STD_CONTAINER(is_std_deque, std::deque)
BEE_DEFINE_STD_CONTAINER(is_std_string, std::basic_string)
BEE_DEFINE_STD_CONTAINER(is_std_set, std::set)
BEE_DEFINE_STD_CONTAINER(is_std_multiset, std::multiset)
BEE_DEFINE_STD_CONTAINER(is_std_unordered_set, std::unordered_set)
BEE_DEFINE_STD_CONTAINER(is_std_unordered_multiset, std::unordered_multiset)
BEE_DEFINE_STD_CONTAINER(is_std_map, std::map)
BEE_DEFINE_STD_CONTAINER(is_std_multimap, std::multimap)
BEE_DEFINE_STD_CONTAINER(is_std_unordered_map, std::unordered_map)
BEE_DEFINE_STD_CONTAINER(is_std_unordered_multimap, std::unordered_multimap)

template<typename T>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template<typename T, std::size_t N>
struct is_stl_container<std::array<T, N>> : std::true_type {};

template<typename T>
constexpr bool is_std_array_v = is_std_array<T>::value;

#undef BEE_DEFINE_STD_CONTAINER

} // namespace bee