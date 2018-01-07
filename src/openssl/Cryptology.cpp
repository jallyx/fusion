#include "Cryptology.h"
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

namespace rsa {

RSA *generate_key()
{
    return RSA_generate_key(1024, RSA_F4, nullptr, nullptr);
}

void free_key(RSA *rsa)
{
    if (rsa != nullptr)
        RSA_free(rsa);
}

std::string encode_pubkey(RSA *rsa)
{
    std::string pubkey;
    unsigned char *pp = nullptr;
    int len = i2d_RSA_PUBKEY(rsa, &pp);
    if (len != -1)
        pubkey.assign((const char *)pp, len);
    if (pp != nullptr)
        free(pp);
    return pubkey;
}

std::string public_encrypt(const std::string &pubkey, const std::string &plaintext)
{
    std::string ciphertext;
    const unsigned char *pp = (const unsigned char *)pubkey.data();
    RSA *rsa = d2i_RSA_PUBKEY(nullptr, &pp, pubkey.size());
    if (rsa != nullptr) {
        ciphertext.resize(RSA_size(rsa));
        int len = RSA_public_encrypt(
            plaintext.size(), (const unsigned char *)plaintext.data(),
            (unsigned char *)ciphertext.data(), rsa, RSA_PKCS1_PADDING);
        ciphertext.resize(len != -1 ? len : 0);
        RSA_free(rsa);
    }
    return ciphertext;
}

std::string private_decrypt(RSA *rsa, const std::string &ciphertext)
{
    std::string plaintext(RSA_size(rsa), '\0');
    int len = RSA_private_decrypt(
        ciphertext.size(), (const unsigned char *)ciphertext.data(),
        (unsigned char *)plaintext.data(), rsa, RSA_PKCS1_PADDING);
    plaintext.resize(len != -1 ? len : 0);
    return plaintext;
}

}

namespace sha {

std::string feed512(const void *data, size_t len)
{
    std::string results(SHA512_DIGEST_LENGTH, '\0');
    SHA512((const unsigned char *)data, len, (unsigned char *)results.data());
    return results;
}

}

namespace md5 {

std::string feed(const void *data, size_t len)
{
    std::string results(MD5_DIGEST_LENGTH, '\0');
    MD5((const unsigned char *)data, len, (unsigned char *)results.data());
    return results;
}

}

namespace base64 {

std::string encode(const void *data, size_t len)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, BIO_new(BIO_s_mem()));
    BIO_write(b64, data, len);
    BIO_flush(b64);
    BUF_MEM *bptr = nullptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string results(bptr->data, bptr->length);
    BIO_free_all(b64);
    return results;
}

std::string decode(const void *data, size_t len)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, BIO_new_mem_buf((void *)data, len));
    std::string results(len / 4 * 3, '\0');
    BIO_read(b64, (void *)results.data(), results.size());
    BIO_free_all(b64);
    return results;
}

}
