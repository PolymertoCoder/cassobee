#pragma once
#include "common.h"
#include <string>
#include <map>

template<typename base_product, typename key_type>
class factory_base
{
public:
    class creator_base
    {
    public:
        virtual base_product* create() = 0;
    };

    template<typename product, typename... create_params>
    requires std::is_base_of_v<base_product, product>
    class creator : public creator_base
    {
    public:
        virtual product* create(create_params&&... params) override
        {
            return new product(std::forward<create_params>(params)...);
        }
    };

    template<typename product, typename... create_params>
    struct register_t
    {
        register_t(const key_type& id)
        {
            assert(_creators.emplace(id, new creator<product, create_params...>()).second);
        }
    };

protected:
    static std::map<key_type, creator_base*> _creators;
};

template<typename base_product, typename key_type>
class factory_impl : public factory_base<base_product, key_type>
                   , public singleton_support<factory_impl<base_product, key_type>>
{
public:
    using factory_base<base_product, key_type>::_creators;

    template<typename... create_params>
    base_product* create(const key_type& id, create_params&&... params)
    {
        if(auto iter = _creators.find(id); iter != _creators.end())
        {
            iter->second->create(std::forward<create_params>(params)...);
        }
        return nullptr;
    }
};

template<typename base_product, typename key_type = std::string>
using factory_template = factory_impl<base_product, key_type>;

#define register_product(factory, id, product, ...) \
    static factory::register_t<product, __VA_ARGS__> _##factory##_##product(id);