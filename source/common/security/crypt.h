#pragma once
#include <string>

class crypt
{
public:
    crypt() = default;
    virtual ~crypt() = default;
    virtual std::string encrypt(const std::string_view& input) = 0;
    virtual std::string decrypt(const std::string_view& input) = 0;  
};

class AES_crypt : public crypt
{
public:
    virtual std::string encrypt(const std::string_view& input) override;
    virtual std::string decrypt(const std::string_view& input) override;  
};