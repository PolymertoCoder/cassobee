#pragma once
#include "common.h"
#include <functional>
#include <string>
#include <utility>

template<typename base_product, typename key_type, typename... products>
class factory_base
{
public:
    template<typename product>
    struct product_stub
    {
        key_type id;
        product  stub;
    };

protected:
    static std::tuple<product_stub<products>...> _stubs;
};

template<typename base_product, typename key_type, typename... products>
class factory_impl : public factory_base<base_product, key_type, products...>
                   , public singleton_support<factory_impl<base_product, key_type>>
{
public:
    using factory_base<base_product, key_type, products...>::_stubs;

    template<typename... create_params>
    base_product* create(const key_type& id, create_params&&... params)
    {
        base_product* product = nullptr;
        [&]<size_t... Is>(std::index_sequence<Is...>&&)
        {
            ((std::get<Is>(_stubs).id == id &&
             (product = new decltype(std::get<Is>(_stubs).stub)(params...))) || ...);
        }(std::make_index_sequence<sizeof...(products)>());
        return product;
    }
};

template<typename base_product, typename key_type = std::string>
using factory_template = factory_impl<base_product, key_type>;

#define register_product(factory, id, product, ...) \
    static factory::register_t<product, __VA_ARGS__> _##factory##_##product(id, );
