#pragma once
#include "type_traits.h"
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

template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

} // namespace bee