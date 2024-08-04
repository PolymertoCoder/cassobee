#pragma once
#include "common.h"
#include "concept.h"
#include "traits.h"
#include <tuple>
#include <utility>

template<typename base_product, typename... products>
class factory_base
{
public:
    template<typename product>
    requires (std::is_base_of_v<base_product, product>)
    struct product_stub
    {
        using type = product;
        constexpr static auto value = cassobee::type_name<product>();
    };

protected:
    static std::tuple<product_stub<products>...> _stubs;
};

template<typename base_product, typename... products>
class factory_impl : public factory_base<base_product, products...>
                   , public singleton_support<factory_impl<base_product, products...>>
{
public:
    using factory_base<base_product, products...>::_stubs;

    template<size_t I>
    using product_wrapper = std::tuple_element_t<I, decltype(_stubs)>;

    template<size_t I, typename... create_params>
    base_product* create_helper(const std::string_view& id, create_params&&... params)
    {
        if constexpr(has_constructor<typename product_wrapper<I>::type, create_params...>)
        {
            if(id == product_wrapper<I>::value)
                return new product_wrapper<I>::type(params...);
        }
        return nullptr;
    }

    template<typename... create_params>
    auto* create(const std::string_view& id, create_params&&... params)
    {
        static_assert(sizeof...(products) > 0);

        base_product* product = nullptr;
        [&]<size_t... Is>(std::index_sequence<Is...>&&)
        {
            ((product = create_helper<Is>(id, params...)) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
        return product;
    }
};

template<typename base_product, typename... products>
using factory_template = factory_impl<base_product, products...>;
