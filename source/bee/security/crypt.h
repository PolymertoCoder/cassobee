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

/*
1.对称加密算法
1.1 AES (Advanced Encryption Standard)
优点: 高效、安全性强，被广泛使用。
缺点: 密钥管理复杂，密钥需要安全分发。
适用场景: 数据库加密、文件加密、通信加密。

2.非对称加密算法
2.1 RSA (Rivest-Shamir-Adleman)
优点: 安全性强，支持数字签名。
缺点: 加密速度慢，密钥长度较长。
适用场景: 密钥交换、数字签名、证书。

3.哈希算法
3.1 SHA-2 (Secure Hash Algorithm 2)
优点: 安全性强，被广泛使用。
缺点: 计算复杂度较高。
适用场景: 数据完整性校验、数字签名。

3.2 MD5 (Message Digest Algorithm 5)
优点: 计算速度快。
缺点: 安全性差，容易发生碰撞。
适用场景: 数据完整性校验（不推荐用于安全性要求高的场景）。

高性能和高安全性: AES 是首选。
资源受限设备: ChaCha20 和 ECC 是不错的选择。
密钥交换和数字签名: RSA 和 ECC 是常用的选择。
数据完整性校验: SHA-2 和 SHA-3 是比较安全的选择。
*/