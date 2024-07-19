#pragma once
#include <algorithm>
#include <mutex>
#include <string_view>
#include <string>
#include <type_traits>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <source_location>

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

template<typename T>
consteval auto type_name_array() -> std::string_view
{
    using std::string_view_literals::operator""sv;
    constexpr std::source_location location;
    constexpr auto function_name = std::string_view(location.current().function_name());
    constexpr auto prefix = "T = "sv;
    constexpr auto suffix = ",;]"sv;

    constexpr auto prepos = std::ranges::search(function_name, prefix);
    static_assert(!prepos.empty());

    constexpr std::ranges::subrange subrange(prepos.end(), function_name.end());
    constexpr auto suf = std::ranges::find_first_of(subrange, suffix);
    static_assert(suf != function_name.end());

    return {prepos.end(), std::distance(prepos.end(), suf)};
}

template<typename T>
struct type_name_holder
{
    static constexpr auto value = type_name_array<T>();
    static auto get() -> const std::string&
    {
        static std::string str = std::string(value.data(), value.size());
        return str;
    }
    static auto get_short() -> const std::string&
    {
        static std::string str;
        static std::once_flag once;
        std::call_once(once, [&]()
        {
            static std::string tmp = std::string(value.data(), value.size());
            size_t pos = tmp.rfind("::");
            str = (pos != std::string::npos) ? tmp.substr(pos + 2) : tmp;
        });
        return str;
    }
};

template<typename T>
const char* type_name()
{
    return type_name_holder<T>::get().data();
}

template<typename T>
const char* short_type_name()
{
    return type_name_holder<T>::get_short().data();
}

}