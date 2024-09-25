#pragma once
#include "traits.h"
#include <cstddef>
#include <utility>

namespace cassobee
{

template<typename T>
concept stl_container = cassobee::is_stl_container_v<T>;

template<typename T>
concept has_reserve = requires { std::declval<T>().reserve(std::declval<size_t>()); };

template<typename T>
concept can_reserve_stl_container = (stl_container<T> && has_reserve<T>);

template<typename T, typename... create_params>
concept has_constructor = requires { T(std::declval<create_params>()...); };

} // namespace cassobee