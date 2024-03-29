#include "openssl_wrapper.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <array>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include "check.h"
#include "utilites/utils.h"

static bool isInitialized = false;

#define CHECK_SSL(v, message) { \
    if (!(v)) { \
        std::vector<char> buffer(270); \
        const auto error = ERR_get_error(); \
        const std::string errorStr = ERR_error_string(error, buffer.data()); \
        const std::string newMessage = std::string(message) + ". " + errorStr; \
        throwErr(newMessage); \
    } \
}

void InitOpenSSL() {
    CHECK(!isInitialized, "Already initialized");
    /*SSL_load_error_strings();
    SSL_library_init();*/
    OpenSSL_add_all_algorithms();
    isInitialized = true;
}

bool isInitOpenSSL() {
    return isInitialized;
}

static std::array<unsigned char, SHA256_DIGEST_LENGTH> doubleSha(const char * data, size_t size) {
    CHECK(isInitialized, "Not initialized");

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash1;
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash2;

    // First pass
    const auto *res1 = SHA256((const unsigned char*)data, size, hash1.data());
    CHECK_SSL(res1 != nullptr, "Incorrect sha256");

    // Second pass
    const auto *res2 = SHA256(hash1.data(), SHA256_DIGEST_LENGTH, hash2.data());
    CHECK_SSL(res2 != nullptr, "Incorrect sha256");

    return hash2;
}

static RsaKey getRsa(const std::string &privKey, const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new_mem_buf((void*)privKey.data(), (int)privKey.size()), BIO_free);
    CHECK_SSL(bio != nullptr, "Incorrect BIO_new_mem_buf");

    const char *pswd = nullptr;
    if (!password.empty()) {
       pswd = password.data();
    }
    const std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY*)>> evp(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, (void*)pswd), EVP_PKEY_free);
    CHECK_SSL(evp != nullptr, "Incorrect password");

    RsaKey rsa(EVP_PKEY_get1_RSA(evp.get()), RSA_free);
    CHECK_SSL(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    const int validateResult = RSA_check_key(rsa.get());
    CHECK_SSL(validateResult == 1, "Rsa key not validate");

    return rsa;
}

PublikKey getPublic(const std::string &privKey, const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    const RsaKey rsa = getRsa(privKey, password);
    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bioPub(BIO_new(BIO_s_mem()), BIO_free);
    const int res5 = i2d_RSA_PUBKEY_bio(bioPub.get(), rsa.get());
    CHECK_SSL(res5 != 0, "Incorrect i2d_RSA_PUBKEY_bio");

    const int keylenPub = BIO_pending(bioPub.get());
    std::vector<unsigned char> pem_key_pub(keylenPub);
    const int res6 = BIO_read(bioPub.get(), pem_key_pub.data(), keylenPub);
    CHECK_SSL(res6 > 0, "Incorrect BIO_read");

    return toHex(std::string(pem_key_pub.begin(), pem_key_pub.end()));
}

PrivateKey createRsaKey(const std::string &password) {
    CHECK(isInitialized, "Not initialized");

    CHECK(password.find('\0') == password.npos, "Incorrect password");

    const int kBits = 2048;

    const std::unique_ptr<BIGNUM, std::function<void(BIGNUM*)>> bne(BN_new(), BN_free);
    const int res1 = BN_set_word(bne.get(), 17);
    CHECK_SSL(res1 != 0, "Incorrect BN_set_word");

    const RsaKey rsa(RSA_new(), RSA_free);
    const int res2 = RSA_generate_key_ex(rsa.get(), kBits, bne.get(), nullptr); // TODO random generator ?
    CHECK_SSL(res2 != 0, "Incorrect RSA_generate_key_ex");

    const std::unique_ptr<BIO, std::function<void(BIO*)>> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (!password.empty()) {
        const int res3 = PEM_write_bio_RSAPrivateKey(bio.get(), rsa.get(), EVP_aes_128_cbc(), (unsigned char*)password.data(), password.size(), nullptr, nullptr);
        CHECK_SSL(res3 != 0, "Incorrect PEM_write_bio_RSAPrivateKey");
    } else {
        const int res3 = PEM_write_bio_RSAPrivateKey(bio.get(), rsa.get(), nullptr, nullptr, 0, nullptr, nullptr);
        CHECK_SSL(res3 != 0, "Incorrect PEM_write_bio_RSAPrivateKey");
    }

    const int keylen = BIO_pending(bio.get());
    std::vector<unsigned char> pem_key(keylen);
    const int res4 = BIO_read(bio.get(), pem_key.data(), keylen);
    CHECK_SSL(res4 > 0, "Incorrect BIO_read");

    return std::string(pem_key.begin(), pem_key.end());
}

static void loadAesKey(const std::array<unsigned char, 32> &ckey, AES_KEY &keyEn) {
    const int res = AES_set_encrypt_key(ckey.data(), 256, &keyEn);
    CHECK_SSL(res == 0, "Incorrect AES_set_encrypt_key");
}

static std::vector<unsigned char> genAesKey(AES_KEY &keyEn) {
    CHECK(RAND_status(), "Rand not seed");
    std::array<unsigned char, 32> key;
    const int res1 = RAND_bytes(key.data(), key.size());
    CHECK_SSL(res1 != 0, "Incorrect RAND_bytes");
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
    const int res1 = RAND_bytes(iv.data(), iv.size());
    CHECK_SSL(res1 != 0, "Incorrect RAND_bytes");
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
    CHECK_SSL(bio != nullptr, "Incorrect BIO_new_mem_buf");

    RsaKey rsa(d2i_RSA_PUBKEY_bio(bio.get(), nullptr), RSA_free);
    CHECK(rsa != nullptr, "Incorrect d2i_RSA_PUBKEY_bio");
    return rsa;
}

static std::string encryptRSA(const RsaKey &pubkey, const std::vector<unsigned char> &message) {
    CHECK(isInitialized, "Not initialized");

    CHECK(pubkey != nullptr, "Incorrect d2i_RSA_PUBKEY_bio");

    std::vector<unsigned char> encrypt(RSA_size(pubkey.get()));
    const int encrypt_len = RSA_public_encrypt(message.size(), (unsigned char*)message.data(), encrypt.data(), pubkey.get(), RSA_PKCS1_OAEP_PADDING);
    CHECK_SSL(encrypt_len != -1, "Incorrect RSA_public_encrypt");
    encrypt.resize(encrypt_len);

    return std::string(encrypt.begin(), encrypt.end());
}

static std::string decryptRsa(const RsaKey &rsa, const std::string &message) {
    CHECK(isInitialized, "Not initialized");

    CHECK(rsa != nullptr, "Incorrect EVP_PKEY_get1_RSA");

    std::vector<unsigned char> decrypt(RSA_size(rsa.get()));
    const int decrypt_len = RSA_private_decrypt(message.size(), (unsigned char*)message.data(), decrypt.data(), rsa.get(), RSA_PKCS1_OAEP_PADDING);
    CHECK_SSL(decrypt_len != -1, "Incorrect RSA_private_decrypt");
    decrypt.resize(decrypt_len);

    return std::string(decrypt.begin(), decrypt.end());
}

static std::string makeMessageHex(const std::string &aes, const std::vector<unsigned char> &iv, const std::vector<unsigned char> &message, const std::vector<unsigned char> &pubkeyPart) {
    const auto writeInt = [](uint64_t value) {
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

    result += writeInt(pubkeyPart.size());
    result += std::string(pubkeyPart.begin(), pubkeyPart.end());

    return toHex(result);
}

static void parseMessageHex(const std::string &message, std::string &aes, std::vector<unsigned char> &iv, std::vector<unsigned char> &result, std::vector<unsigned char> &pubkeyPart) {
    const std::string normMessage = fromHex(message);

    const auto readInt = [](auto &currIter, auto endIter) {
        const int len = sizeof(uint64_t);
        CHECK(std::distance(currIter, endIter) >= len, "Incorrect message");
        uint64_t result = 0;
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

    const size_t pubkeySize = readInt(curIter, endIter);
    CHECK((size_t)std::distance(curIter, endIter) >= pubkeySize, "Incorrect message");
    pubkeyPart.assign(curIter, curIter + pubkeySize);
    curIter += pubkeySize;
}

const static std::vector<unsigned char> calcPubkeyPart(const std::string &pubkeyString) {
    const auto doubleShaPubkey = doubleSha(pubkeyString.data(), pubkeyString.size());
    const std::vector<unsigned char> pubkeyPart(doubleShaPubkey.begin(), doubleShaPubkey.begin() + 8);
    return pubkeyPart;
}

std::string encrypt(const RsaKey &publicKey, const std::string &message, const std::string &pubkeyString) {
    CHECK(isInitialized, "Not initialized");

    AES_KEY aesKey;
    const std::vector<unsigned char> aesVect = genAesKey(aesKey);
    const EncryptAes encryptAesResult = ::encryptAes(message, aesKey);

    const std::string aes = encryptRSA(publicKey, aesVect);

    const std::vector<unsigned char> pubkeyPart = calcPubkeyPart(pubkeyString);

    return makeMessageHex(aes, encryptAesResult.iv, encryptAesResult.message, pubkeyPart);
}

std::string decrypt(const RsaKey &privkey, const std::string &message, const std::string &pubkeyString) {
    CHECK(isInitialized, "Not initialized");

    std::string aesEncrypted;
    std::vector<unsigned char> iv;
    std::vector<unsigned char> msg;
    std::vector<unsigned char> pubkeyPartMessage;
    parseMessageHex(message, aesEncrypted, iv, msg, pubkeyPartMessage);

    const std::vector<unsigned char> pubkeyPart = calcPubkeyPart(pubkeyString);
    CHECK(pubkeyPart == pubkeyPartMessage, "Pubkey encrypted message and current pubkey not equals");

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

bool validatePublicKey(const RsaKey &privateKey, const RsaKey &publicKey) {
    return BN_cmp(publicKey->n, privateKey->n) == 0;
}
