#pragma once
#include <tuple>
#include <utility>
#include "glog.h"
#include "log.h"
#include "traits.h"

namespace bee
{

template<typename base_product, typename... products>
class factory_base
{
public:
    template<typename product>
    requires (std::is_base_of_v<base_product, product>)
    struct product_stub
    {
        using type = product;
        constexpr static auto value = bee::short_type_name<product>();
    };

    [[nodiscard]] auto infomation()
    {
        return []<size_t... Is>(std::index_sequence<Is...>&&)
        {
            return ((std::string(std::tuple_element_t<Is, decltype(_stubs)>::value) + " ") + ...);
        }(std::make_index_sequence<sizeof...(products)>());
    }

protected:
    static std::tuple<product_stub<products>...> _stubs;
};

template<typename base_product, typename... products>
class factory_impl : public factory_base<base_product, products...>
{
protected:
    using factory_base<base_product, products...>::_stubs;

    template<size_t I>
    using product_wrapper = std::tuple_element_t<I, decltype(_stubs)>;

    template<bee::string_literal id, typename... create_params>
    [[nodiscard]] static consteval bool static_check()
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>&&)
        {
            return ((id == product_wrapper<Is>::value &&
                     std::constructible_from<typename product_wrapper<Is>::type, create_params...>) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
    }

    template<size_t I, typename... create_params>
    [[nodiscard]] static base_product* create_helper(create_params&&... params)
    {
        if constexpr(std::constructible_from<typename product_wrapper<I>::type, create_params...>)
        {
            return new product_wrapper<I>::type(std::forward<create_params>(params)...);
        }
        else
        {
            local_log("factory %s cannot find product constructor: \"%s(%s)\".",
                    std::string(factory_name).data(), std::string(product_wrapper<I>::value).data(),
                    bee::join(", ", type_name_string<create_params>()...).data());
        }
        return nullptr;
    }

public:
    static constexpr auto factory_name = bee::type_name<factory_impl<base_product, products...>>();

    template<typename... create_params>
    [[nodiscard]] static auto* create(const std::string_view& id, create_params&&... params)
    {
        static_assert(sizeof...(products) > 0);

        base_product* product = nullptr;
        [&]<size_t... Is>(std::index_sequence<Is...>&&)
        {
            (((id == product_wrapper<Is>::value) &&
               (product = create_helper<Is>(std::forward<create_params>(params)...))) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
        return product;
    }

    template<bee::string_literal id, typename... create_params>
    [[nodiscard]] static auto* create2(create_params&&... params)
    {
        static_assert(static_check<id, create_params...>());
        return create(id, std::forward<create_params>(params)...);
    }
};

template<typename base_product, typename... products>
using factory_template = factory_impl<base_product, products...>;

}