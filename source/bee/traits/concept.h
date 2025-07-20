#pragma once
#include "type_traits.h"
#include <cstddef>
#include <utility>

namespace bee
{

template<typename T>
concept stl_container = bee::is_stl_container_v<T>;

template<typename T>
concept continous_stl_container = is_std_array_v<T> || is_std_string_v<T> || is_std_vector_v<T>;

template<typename T>
concept has_reserve = requires { std::declval<T>().reserve(std::declval<size_t>()); };

template<typename T>
concept can_reserve_stl_container = (stl_container<T> && has_reserve<T>);

template<typename T>
concept swappable = requires {
    { std::swap(std::declval<T&>(), std::declval<T&>()) } -> std::same_as<void>;
};

template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept standard_layout = std::is_standard_layout_v<T>;

template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

template<typename T>
concept pod = std::is_standard_layout_v<T> && std::is_trivial_v<T>;

template<typename T>
concept not_pod_stl_container = stl_container<T> && !pod<T>;

} // namespace bee