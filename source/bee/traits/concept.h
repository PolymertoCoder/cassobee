#pragma once
#include "traits.h"
#include <cstddef>
#include <utility>

namespace bee
{

template<typename T>
concept stl_container = bee::is_stl_container_v<T>;

template<typename T>
concept has_reserve = requires { std::declval<T>().reserve(std::declval<size_t>()); };

template<typename T>
concept can_reserve_stl_container = (stl_container<T> && has_reserve<T>);

} // namespace bee