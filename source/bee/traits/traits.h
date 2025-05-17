#pragma once
#include <algorithm>
#include <string_view>
#include <string>
#include <type_traits>
#include <source_location>

namespace bee
{

template<typename... T>
struct typelist {};

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
    if constexpr(constexpr auto pos = ref.rfind("::"); pos != std::string_view::npos)
    {
        constexpr auto diff = pos + 2;
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
struct string_literal
{
    consteval string_literal(const char (&str)[N]) // 数组引用，不会退化成指针，能保留数组的大小信息
    {
        std::copy_n(str, N, data);
    }
    consteval string_literal(const string_literal& rhs)
    {
        std::copy_n(rhs.data, N, data);
    }
    consteval operator std::string_view() const
    {
        return std::string_view(data, N - 1); // N-1 to exclude the null terminator
    }
    consteval bool operator==(std::string_view other) const
    {
        return std::string_view(data, N - 1) == other;
    }
    char data[N];
};

template<size_t N>
string_literal(const char (&str)[N]) -> string_literal<N>;

} // namespace bee