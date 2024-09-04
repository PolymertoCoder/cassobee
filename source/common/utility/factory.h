#pragma once
#include "common.h"
#include "concept.h"
#include "log.h"
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
        constexpr static auto value = cassobee::short_type_name<product>();
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

    static consteval bool product_exists(const std::string_view& id)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>&&)
        {
            return ((id == std::tuple_element_t<Is, decltype(_stubs)>::value) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
    }
};

template<typename base_product, typename... products>
class factory_impl : public factory_base<base_product, products...>
                   , public singleton_support<factory_impl<base_product, products...>>
{
public:
    using factory_base<base_product, products...>::_stubs;
    using factory_base<base_product, products...>::product_exists;

    static constexpr auto factory_name = cassobee::type_name<factory_impl<base_product, products...>>();

    template<size_t I>
    using product_wrapper = std::tuple_element_t<I, decltype(_stubs)>;

    template<size_t I, typename... create_params>
    base_product* create_helper(const std::string_view& id, create_params&&... params)
    {
        if constexpr(cassobee::has_constructor<typename product_wrapper<I>::type, create_params...>)
        {
            if(id == product_wrapper<I>::value)
                return new product_wrapper<I>::type(std::forward<create_params>(params)...);
        }
        else
        {
            local_log("factory %s cannot find product %s constructor.", std::string(factory_name).data(), id.data());
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
            ((product = create_helper<Is>(id, std::forward<create_params>(params)...)) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
        return product;
    }

    template<cassobee::string_literal id, typename... create_params>
    auto* create2(create_params&&... params)
    {
        static_assert(product_exists(std::string_view(id)));
        return create(std::string_view(id), std::forward<create_params>(params)...);
    }
};

template<typename base_product, typename... products>
using factory_template = factory_impl<base_product, products...>;
