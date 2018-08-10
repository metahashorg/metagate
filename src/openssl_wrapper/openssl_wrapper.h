#ifndef OPENSSL_WRAPPER_H
#define OPENSSL_WRAPPER_H

#include <string>
#include <memory>
#include <functional>

void InitOpenSSL();

using PrivateKey = std::string;
using PublikKey = std::string;

typedef struct rsa_st RSA;
using RsaKey = std::unique_ptr<RSA, std::function<void(RSA*)>>;

PublikKey getPublic(const std::string &privKey, const std::string &password);

PrivateKey createRsaKey(const std::string &password);

std::string encrypt(const RsaKey &publicKey, const std::string &message);

std::string decrypt(const RsaKey &privkey, const std::string &message);

RsaKey getPublicRsa(const PublikKey &pKey);

RsaKey getPrivateRsa(const std::string &privkey, const std::string &password);

#endif // OPENSSL_WRAPPER_H
