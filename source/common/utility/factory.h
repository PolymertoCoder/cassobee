#pragma once
#include <string>
#include <map>

enum FactoryCreateType
{
    CREATE_BY_CREATOR,
    CREATE_BY_STUB
};

template<typename base_product, typename key_type, FactoryCreateType create_type>
class factory_base;

template<typename base_product, typename key_type>
class factory_base<base_product, key_type, CREATE_BY_CREATOR>
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
        register_t(const key_type& id, create_params&&... params)
        {
            assert(_creators.emplace(id, new creator<product, create_params...>()).second);
        }
    };

protected:
    static std::map<key_type, creator_base*> _creators;
};


template<typename base_product, typename key_type>
class factory_base<base_product, key_type, CREATE_BY_STUB>
{
public:
    class product_wrapper_base
    {
    public:
        virtual base_product* get_stub() = 0;
    };

    template<typename product>
    requires std::is_base_of_v<base_product, product>
    class product_wrapper : public product_wrapper_base
    {
    public:
        product_wrapper(product* stub) : _stub(stub) {}
        virtual product* get_stub() override { return _stub; }
    private:
        product* _stub = nullptr;
    };

    struct register_t
    {
        register_t(const key_type& id, product_wrapper_base* creator)
        {
            assert(_stubs.emplace(id, creator).second);
        }
    };

protected:
    static std::map<key_type, product_wrapper_base*> _stubs;
};

template<typename base_product, typename key_type, FactoryCreateType create_type>
class factory_impl;

template<typename base_product, typename key_type>
class factory_impl<base_product, key_type, CREATE_BY_CREATOR> : public factory_base<base_product, key_type, CREATE_BY_CREATOR>
{
public:
    using factory_base<base_product, key_type, CREATE_BY_CREATOR>::_creators;

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

template<typename base_product, typename key_type>
class factory_impl<base_product, key_type, CREATE_BY_STUB> : public factory_base<base_product, key_type, CREATE_BY_STUB>
{
public:
    using factory_base<base_product, key_type, CREATE_BY_STUB>::_stubs;

    template<typename... create_params>
    base_product* create(const key_type& id, create_params&&... params)
    {
        if(auto iter = _stubs.find(id); iter != _stubs.end())
        {
            decltype(*(iter->second->get_stub()))(std::forward<create_params>(params)...);
        }
        return nullptr;
    }
};

template<typename base_product, typename key_type = std::string, FactoryCreateType create_type = CREATE_BY_CREATOR>
using factory_template = factory_impl<base_product, key_type, create_type>;
