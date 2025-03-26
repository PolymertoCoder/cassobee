#pragma once
#include <format>
#include "stringfy.h"

namespace std
{

// std::pair
template <typename T1, typename T2> struct formatter<std::pair<T1, T2>>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const std::pair<T1, T2>& p, format_context& ctx) const
    {
        return format_to(ctx.out(), "{}", bee::to_string(p));
    }
};

// STL 容器
template <bee::stl_container Container> struct formatter<Container>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const Container& c, format_context& ctx) const
    {
        return format_to(ctx.out(), "{}", bee::to_string(c));
    }
};

// tuple
template <typename... Args> struct formatter<std::tuple<Args...>>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const std::tuple<Args...>& t, format_context& ctx) const
    {
        return format_to(ctx.out(), "{}", bee::to_string(t));
    }
};
    

} // namespace std