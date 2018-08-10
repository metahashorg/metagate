#include "openssl_wrapper.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <array>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

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

static RsaKey getRsa(const std::string &privKey, const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new_mem_buf((void*)privKey.data(), (int)privKey.size()), BIO_free);
    CHECK(bio != nullptr, "Incorrect BIO_new_mem_buf");

    const char *pswd = nullptr;
    if (!password.empty()) {
       pswd = password.data();
    }
    const std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY*)>> evp(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, (void*)pswd), EVP_PKEY_free);
    CHECK(evp != nullptr, "Incorrect password");

    RsaKey rsa(EVP_PKEY_get1_RSA(evp.get()), RSA_free);
    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    return rsa;
}

PublikKey getPublic(const std::string &privKey, const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    const RsaKey rsa = getRsa(privKey, password);
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

    const RsaKey rsa(RSA_new(), RSA_free);
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

static void loadAesKey(const std::array<unsigned char, 32> &ckey, AES_KEY &keyEn) {
    AES_set_encrypt_key(ckey.data(), 256, &keyEn);
}

static std::vector<unsigned char> genAesKey(AES_KEY &keyEn) {
    CHECK(RAND_status(), "Rand not seed");
    std::array<unsigned char, 32> key;
    RAND_bytes(key.data(), key.size());
    loadAesKey(key, keyEn);
    return std::vector<unsigned char>(key.begin(), key.end());
}

struct EncryptAes {
    std::vector<unsigned char> message;
    std::vector<unsigned char> iv;
};

static EncryptAes encryptAes(const std::string &message, const AES_KEY &keyEn) {
    EncryptAes result;

    CHECK(RAND_status(), "Rand not seed");
    std::array<unsigned char, 32> iv;
    RAND_bytes(iv.data(), iv.size());
    result.iv.assign(iv.begin(), iv.end()); // iv должен сохраняться тут, так как дальше он будет перезаписываться
    result.message.resize(message.size(), 0);

    int num = 0;
    AES_cfb128_encrypt((const unsigned char*)message.data(), result.message.data(), message.size(), &keyEn, iv.data(), &num, AES_ENCRYPT);
    return result;
}

static std::string decryptAes(const std::vector<unsigned char> &message, const AES_KEY &keyEn, const std::array<unsigned char, 32> &iv) {
    std::vector<unsigned char> result(message.size(), 0);
    std::array<unsigned char, 32> ivCopy = iv;
    int num = 0;
    AES_cfb128_encrypt((const unsigned char*)message.data(), result.data(), message.size(), &keyEn, ivCopy.data(), &num, AES_DECRYPT);
    return std::string(result.begin(), result.end());
}

static RsaKey getRsaPublikKey(const std::string &pubkey) {
    CHECK(isInitialized, "Not initialized");

    const std::string normPubKey = fromHex(pubkey);
    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new_mem_buf((void*)normPubKey.data(), (int)normPubKey.size()), BIO_free);
    CHECK(bio != nullptr, "Incorrect BIO_new_mem_buf");

    RsaKey rsa(d2i_RSA_PUBKEY_bio(bio.get(), nullptr), RSA_free);
    CHECK(rsa != nullptr, "Incorrect d2i_RSA_PUBKEY_bio");
    return rsa;
}

static std::string encryptRSA(const RsaKey &pubkey, const std::vector<unsigned char> &message) {
    CHECK(isInitialized, "Not initialized");

    CHECK(pubkey != nullptr, "Incorrect d2i_RSA_PUBKEY_bio");

    std::vector<unsigned char> encrypt(RSA_size(pubkey.get()));
    const int encrypt_len = RSA_public_encrypt(message.size(), (unsigned char*)message.data(), encrypt.data(), pubkey.get(), RSA_PKCS1_OAEP_PADDING);
    CHECK(encrypt_len != -1, "Incorrect RSA_public_encrypt");
    encrypt.resize(encrypt_len);

    return std::string(encrypt.begin(), encrypt.end());
}

static std::string decryptRsa(const RsaKey &rsa, const std::string &message) {
    CHECK(isInitialized, "Not initialized");

    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    std::vector<unsigned char> decrypt(RSA_size(rsa.get()));
    const int decrypt_len = RSA_private_decrypt(message.size(), (unsigned char*)message.data(), decrypt.data(), rsa.get(), RSA_PKCS1_OAEP_PADDING);
    CHECK(decrypt_len != -1, "Incorrect RSA_public_encrypt");
    decrypt.resize(decrypt_len);

    return std::string(decrypt.begin(), decrypt.end());
}

static std::string makeMessageHex(const std::string &aes, const std::vector<unsigned char> &iv, const std::vector<unsigned char> &message) {
    const auto writeInt = [](size_t value) {
        std::string res;
        const int len = sizeof(value);
        for (int i = 0; i < len; i++) {
            res = std::string(1, (char)(value % 256)) + res;
            value /= 256;
        }
        return res;
    };

    std::string result;
    result += writeInt(aes.size());
    result += aes;

    result += writeInt(iv.size());
    result += std::string(iv.begin(), iv.end());

    result += writeInt(message.size());
    result += std::string(message.begin(), message.end());

    return toHex(result);
}

static void parseMessageHex(const std::string &message, std::string &aes, std::vector<unsigned char> &iv, std::vector<unsigned char> &result) {
    const std::string normMessage = fromHex(message);

    const auto readInt = [](auto &currIter, auto endIter) {
        const int len = sizeof(size_t);
        CHECK(std::distance(currIter, endIter) >= len, "Incorrect message");
        size_t result = 0;
        for (int i = 0; i < len; i++) {
            result *= 256;
            result += (unsigned char)(*currIter);
            currIter++;
        }
        return result;
    };

    auto curIter = normMessage.cbegin();
    auto endIter = normMessage.cend();

    const size_t aesSize = readInt(curIter, endIter);
    CHECK((size_t)std::distance(curIter, endIter) >= aesSize, "Incorrect message");
    aes.assign(curIter, curIter + aesSize);
    curIter += aesSize;

    const size_t ivSize = readInt(curIter, endIter);
    CHECK((size_t)std::distance(curIter, endIter) >= ivSize, "Incorrect message");
    iv.assign(curIter, curIter + ivSize);
    curIter += ivSize;

    const size_t messageSize = readInt(curIter, endIter);
    CHECK((size_t)std::distance(curIter, endIter) >= messageSize, "Incorrect message");
    result.assign(curIter, curIter + messageSize);
    curIter += messageSize;
}

std::string encrypt(const RsaKey &publicKey, const std::string &message) {
    CHECK(isInitialized, "Not initialized");

    AES_KEY aesKey;
    const std::vector<unsigned char> aesVect = genAesKey(aesKey);
    const EncryptAes encryptAesResult = ::encryptAes(message, aesKey);

    const std::string aes = encryptRSA(publicKey, aesVect);

    return makeMessageHex(aes, encryptAesResult.iv, encryptAesResult.message);
}

std::string decrypt(const RsaKey &privkey, const std::string &message) {
    CHECK(isInitialized, "Not initialized");

    std::string aesEncrypted;
    std::vector<unsigned char> iv;
    std::vector<unsigned char> msg;
    parseMessageHex(message, aesEncrypted, iv, msg);

    const std::string aes = decryptRsa(privkey, aesEncrypted);

    AES_KEY aesKey;
    std::array<unsigned char, 32> aesArr;
    CHECK(aes.size() == aesArr.size(), "Incorrect aes");
    std::copy_n(aes.begin(), aesArr.size(), aesArr.begin());
    loadAesKey(aesArr, aesKey);

    std::array<unsigned char, 32> ivArr;
    CHECK(iv.size() == ivArr.size(), "Incorrect iv");
    std::copy_n(iv.begin(), ivArr.size(), ivArr.begin());
    const std::string result = decryptAes(msg, aesKey, ivArr);

    return result;
}

RsaKey getPublicRsa(const PublikKey &pKey) {
    return getRsaPublikKey(pKey);
}

RsaKey getPrivateRsa(const std::string &privkey, const std::string &password) {
    return getRsa(privkey, password);
}
