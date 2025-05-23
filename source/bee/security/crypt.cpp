#include "crypt.h"
#include "modes.h"
#include "filters.h"
#include "glog.h"

namespace bee
{

using byte = unsigned char;

std::string AES_crypt::encrypt(const std::string_view& plaintext)
{
    return encrypt(plaintext.data(), plaintext.size());
}

std::string AES_crypt::encrypt(const char* plaintext, size_t size)
{
    std::string ciphertext;
    try
    {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
        encryption.SetKeyWithIV(_key, _key.size(), _iv);

        CryptoPP::StringSource ss(reinterpret_cast<const byte*>(plaintext), size, true,
            new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(ciphertext)));
    }
    catch(const CryptoPP::Exception& e)
    {
        local_log("AES Encryption error: %s", e.what());
        throw;
    }
    return ciphertext;
}

std::string AES_crypt::decrypt(const std::string_view& ciphertext)
{
    return decrypt(ciphertext.data(), ciphertext.size());
}

std::string AES_crypt::decrypt(const char* ciphertext, size_t size)
{
    std::string decryptedtext;
    try
    {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
        decryption.SetKeyWithIV(_key, _key.size(), _iv);

        CryptoPP::StringSource ss(reinterpret_cast<const byte*>(ciphertext), size, true,
                new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::StringSink(decryptedtext)));
    }
    catch (const CryptoPP::Exception& e)
    {
        local_log("AES Decryption error: %s", e.what());
        throw;
    }
    return decryptedtext;
}

std::string RSA_crypt::encrypt(const std::string_view& plaintext)
{
    return encrypt(plaintext.data(), plaintext.size());
}

std::string RSA_crypt::encrypt(const char* plaintext, size_t size)
{
    std::string ciphertext;
    try
    {
        CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(_public_key);
        CryptoPP::StringSource ss(reinterpret_cast<const byte*>(plaintext), size, true,
            new CryptoPP::PK_EncryptorFilter(_rng, encryptor, new CryptoPP::StringSink(ciphertext)));
    }
    catch(const CryptoPP::Exception& e)
    {
        local_log("RSA Encryption error: %s", e.what());
        throw;
    }
    return ciphertext;
}

std::string RSA_crypt::decrypt(const std::string_view& ciphertext)
{
    return decrypt(ciphertext.data(), ciphertext.size());
}

std::string RSA_crypt::decrypt(const char* ciphertext, size_t size)
{
    std::string decryptedtext;
    try
    {
        CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(_private_key);
        CryptoPP::StringSource ss(reinterpret_cast<const byte*>(ciphertext), size, true,
            new CryptoPP::PK_DecryptorFilter(_rng, decryptor, new CryptoPP::StringSink(decryptedtext)));
    }
    catch(const CryptoPP::Exception& e)
    {
        local_log("RSA Decryption error: %s", e.what());
        throw;
    }
    return decryptedtext;
}

} // namespace bee