#include "openssl_wrapper.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <QString>
#include <QByteArray>

#include "check.h"
#include "utils.h"

static bool isInitialized = false;

void InitOpenSSL() {
    CHECK(!isInitialized, "Already initialized");
    /*SSL_load_error_strings();
    SSL_library_init();*/
    OpenSSL_add_all_algorithms();
    isInitialized = true;
}

static std::unique_ptr<RSA, std::function<void(RSA*)>> getRsa(const std::string &privKey, const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new_mem_buf((void*)privKey.data(), (int)privKey.size()), BIO_free);
    CHECK(bio != nullptr, "Incorrect BIO_new_mem_buf");

    const char *pswd = nullptr;
    if (!password.empty()) {
       pswd = password.data();
    }
    const std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY*)>> evp(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, (void*)pswd), EVP_PKEY_free);
    CHECK(evp != nullptr, "Incorrect password");

    std::unique_ptr<RSA, std::function<void(RSA*)>> rsa(EVP_PKEY_get1_RSA(evp.get()), RSA_free);
    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    return rsa;
}

PublikKey getPublic(const std::string &privKey, const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    const std::unique_ptr<RSA, std::function<void(RSA*)>> rsa = getRsa(privKey, password);
    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bioPub(BIO_new(BIO_s_mem()), BIO_free);
    const bool res5 = i2d_RSA_PUBKEY_bio(bioPub.get(), rsa.get());
    CHECK(res5, "Incorrect i2d_RSA_PUBKEY_bio");

    const int keylenPub = BIO_pending(bioPub.get());
    std::vector<unsigned char> pem_key_pub(keylenPub);
    const bool res6 = BIO_read(bioPub.get(), pem_key_pub.data(), keylenPub);
    CHECK(res6, "Incorrect BIO_read");

    return toHex(std::string(pem_key_pub.begin(), pem_key_pub.end()));
}

PrivateKey createRsaKey(const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    CHECK(password.find('\0') == password.npos, "Incorrect password");

    const int kBits = 2048;

    const std::unique_ptr<BIGNUM, std::function<void(BIGNUM*)>> bne(BN_new(), BN_free);
    const bool res1 = BN_set_word(bne.get(), 17);
    CHECK(res1, "Incorrect BN_set_word");

    const std::unique_ptr<RSA, std::function<void(RSA*)>> rsa(RSA_new(), RSA_free);
    const bool res2 = RSA_generate_key_ex(rsa.get(), kBits, bne.get(), nullptr); // TODO random generator ?
    CHECK(res2, "Incorrect RSA_generate_key_ex");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (!password.empty()) {
        const bool res3 = PEM_write_bio_RSAPrivateKey(bio.get(), rsa.get(), EVP_aes_128_cbc(), (unsigned char*)password.data(), password.size(), nullptr, nullptr);
        CHECK(res3, "Incorrect PEM_write_bio_RSAPrivateKey");
    } else {
        const bool res3 = PEM_write_bio_RSAPrivateKey(bio.get(), rsa.get(), nullptr, nullptr, 0, nullptr, nullptr);
        CHECK(res3, "Incorrect PEM_write_bio_RSAPrivateKey");
    }

    const int keylen = BIO_pending(bio.get());
    std::vector<unsigned char> pem_key(keylen);
    const bool res4 = BIO_read(bio.get(), pem_key.data(), keylen);
    CHECK(res4, "Incorrect BIO_read");

    return std::string(pem_key.begin(), pem_key.end());
}

std::string encrypt(const std::string &pubkey, const std::string &message) {
    CHECK(isInitialized, "Not initialized");

    const std::string normPubKey = fromHex(pubkey);
    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new_mem_buf((void*)normPubKey.data(), (int)normPubKey.size()), BIO_free);
    CHECK(bio != nullptr, "Incorrect BIO_new_mem_buf");

    const std::unique_ptr<RSA, std::function<void(RSA*)>> rsa(d2i_RSA_PUBKEY_bio(bio.get(), nullptr), RSA_free);
    CHECK(rsa != nullptr, "Incorrect d2i_RSA_PUBKEY_bio");

    std::vector<unsigned char> encrypt(RSA_size(rsa.get()));
    const int encrypt_len = RSA_public_encrypt(message.size(), (unsigned char*)message.data(), encrypt.data(), rsa.get(), RSA_PKCS1_OAEP_PADDING);
    CHECK(encrypt_len != -1, "Incorrect RSA_public_encrypt");
    encrypt.resize(encrypt_len);

    return toHex(std::string(encrypt.begin(), encrypt.end()));
}

std::string decrypt(const std::string &privkey, const std::string &password, const std::string &message) {
    CHECK(isInitialized, "Not initialized");

    const std::string normMessage = fromHex(message);

    const std::unique_ptr<RSA, std::function<void(RSA*)>> rsa = getRsa(privkey, password);
    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    std::vector<unsigned char> decrypt(RSA_size(rsa.get()));
    const int decrypt_len = RSA_private_decrypt(normMessage.size(), (unsigned char*)normMessage.data(), decrypt.data(), rsa.get(), RSA_PKCS1_OAEP_PADDING);
    CHECK(decrypt_len != -1, "Incorrect RSA_public_encrypt");
    decrypt.resize(decrypt_len);

    return std::string(decrypt.begin(), decrypt.end());
}
