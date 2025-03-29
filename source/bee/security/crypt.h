#pragma once
#include <string>
#include <string_view>
#include "aes.h"
#include "osrng.h"
#include "secblockfwd.h"
#include "rsa.h"

namespace bee
{

class crypt
{
public:
    crypt() = default;
    virtual ~crypt() = default;
    virtual std::string encrypt(const std::string_view& plaintext)  = 0;
    virtual std::string encrypt(const char* plaintext, size_t size) = 0;

    virtual std::string decrypt(const std::string_view& ciphertext)  = 0;
    virtual std::string decrypt(const char* ciphertext, size_t size) = 0;  
};

class AES_crypt : public crypt
{
public:
    virtual std::string encrypt(const std::string_view& plaintext)  override;
    virtual std::string encrypt(const char* plaintext, size_t size) override;

    virtual std::string decrypt(const std::string_view& ciphertext)  override;
    virtual std::string decrypt(const char* ciphertext, size_t size) override;

private:
    CryptoPP::AutoSeededRandomPool _rng; // random number generators
    CryptoPP::SecByteBlock _key{CryptoPP::AES::DEFAULT_KEYLENGTH};
    CryptoPP::SecByteBlock _iv{CryptoPP::AES::BLOCKSIZE};
};

class RSA_crypt : public crypt
{
public:
    virtual std::string encrypt(const std::string_view& plaintext)  override;
    virtual std::string encrypt(const char* plaintext, size_t size) override;

    virtual std::string decrypt(const std::string_view& ciphertext)  override;
    virtual std::string decrypt(const char* ciphertext, size_t size) override;

private:
    CryptoPP::AutoSeededRandomPool _rng; // random number generators
    CryptoPP::RSA::PrivateKey _private_key;
    CryptoPP::RSA::PublicKey _public_key;
};

} // namespace bee