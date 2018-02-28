#pragma once

#include <openssl/rsa.h>
#include <string>

namespace rsa {

RSA *generate_key();
void free_key(RSA *rsa);

std::string encode_pubkey(RSA *rsa);

std::string public_encrypt(const std::string &pubkey, const void *data, size_t len);
std::string private_decrypt(RSA *rsa, const void *data, size_t len);

}

namespace hex {

std::string dump(const void *data, size_t len);

}

namespace sha {

std::string feed256(const void *data, size_t len);
std::string feed512(const void *data, size_t len);

}

namespace md5 {

std::string feed(const void *data, size_t len);

}

namespace base64 {

std::string encode(const void *data, size_t len);
std::string decode(const void *data, size_t len);

}
