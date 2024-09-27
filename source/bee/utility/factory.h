#pragma once
#include "concept.h"
#include "log.h"
#include "stringfy.h"
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
};

template<typename base_product, typename... products>
class factory_impl : public factory_base<base_product, products...>
{
protected:
    using factory_base<base_product, products...>::_stubs;

    template<size_t I>
    using product_wrapper = std::tuple_element_t<I, decltype(_stubs)>;

    template<size_t I, typename... create_params>
    [[nodiscard]] static consteval bool static_check_helper(const std::string_view& id)
    {
        return id == product_wrapper<I>::value &&
                cassobee::has_constructor<typename product_wrapper<I>::type, create_params...>;
    }

    template<typename... create_params>
    [[nodiscard]] static consteval bool static_check(const std::string_view& id)
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>&&)
        {
            return ((static_check_helper<Is, create_params...>(id)) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
    }

    template<size_t I, typename... create_params>
    [[nodiscard]] static base_product* create_helper(const std::string_view& id, create_params&&... params)
    {
        if constexpr(cassobee::has_constructor<typename product_wrapper<I>::type, create_params...>)
        {
            if(id == product_wrapper<I>::value)
            {
                return new product_wrapper<I>::type(std::forward<create_params>(params)...);
            }
        }
        else
        {
            ERRORLOG("factory %s cannot find product %s constructor, params=%s.", std::string(factory_name).data(), id.data(), cassobee::to_string(std::tie(std::forward<create_params>(params)...)).data());
        }
        return nullptr;
    }

public:
    static constexpr auto factory_name = cassobee::type_name<factory_impl<base_product, products...>>();

    template<typename... create_params>
    [[nodiscard]] static auto* create(const std::string_view& id, create_params&&... params)
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
    [[nodiscard]] static auto* create2(create_params&&... params)
    {
        static_assert(static_check<create_params...>(id));
        return create(id, std::forward<create_params>(params)...);
    }
};

template<typename base_product, typename... products>
using factory_template = factory_impl<base_product, products...>;