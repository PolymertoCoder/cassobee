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
    constexpr auto suffix = ";]"sv;

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
};

template<typename T>
consteval auto type_name() -> std::string_view
{
    return type_name_holder<T>::value;
}

template<typename T>
consteval auto short_type_name() -> std::string_view
{
    constexpr auto& ref = type_name_holder<T>::value;
    constexpr auto begin = ref.find('<'); // T是模板类
    constexpr auto len = (begin != std::string_view::npos) ? begin : ref.size();
    if constexpr(constexpr auto end = ref.find("::", 0, len); end != std::string_view::npos)
    {
        constexpr auto diff = end + 2;
        return std::string_view(ref.data() + diff, ref.size() - diff);
    }
    else
    {
        return std::string_view(ref.data(), ref.size());
    }
}

template<typename T>
auto type_name_string() -> std::string
{
    return std::string(type_name<T>());
}

template<typename T>
auto short_type_name_string() -> std::string
{
    return std::string(short_type_name<T>());
}

template<size_t N>
struct strTP
{
    constexpr strTP(const char str[N]) { for(size_t i = 0; i < N; ++i) data[i] = str[N]; }
    constexpr bool operator==(const std::string_view& view)
    {
        if(view.size() != N) return false;
        for(size_t i = 0; i < N; ++i)
        {
            if(view[i] != data[i])
                return false;
        }
        return true;
    }
    char data[N];
};

} // namespace cassobee