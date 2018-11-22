#include "wif.h"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <array>

#include "cryptopp/oids.h"
#include "cryptopp/osrng.h"
#include "cryptopp/ripemd.h"
#include <cryptopp/ccm.h>

#include "secp256k1/include/secp256k1_recovery.h"

#include "../ethtx/scrypt/libscrypt.h"

#include "check.h"

#include "Base58.h"
#include "ethtx/cert.h"
#include "ethtx/const.h"
#include "ethtx/utils2.h"

secp256k1_context const* getCtx();

std::string WIFToPrivkey(const std::string& wif, bool& isCompressed) {
    std::vector<unsigned char> decoded;
    const bool res = DecodeBase58(wif.c_str(), decoded);
    CHECK_TYPED(res, TypeErrors::PRIVATE_KEY_ERROR, "dont decode wif key");
    isCompressed = (decoded.size() == 38 && decoded[33] == 0x1);
    CHECK_TYPED(decoded.size() >= 33, TypeErrors::PRIVATE_KEY_ERROR, "dont decode wif key");
    const std::string rawprivkey = std::string((char*)decoded.data() + 1, 32);
    return rawprivkey;
}

std::string PrivKeyToPubKey(const std::string& rawprivkey) {
    secp256k1_pubkey pubkey;
    const bool res = secp256k1_ec_pubkey_create(getCtx(), &pubkey, (const uint8_t*)rawprivkey.data());
    CHECK_TYPED(res, TypeErrors::PRIVATE_KEY_ERROR, "dont create pubkey");
    uint8_t keybuf[EC_PUB_KEY_LENGTH];
    size_t keybufsize = EC_PUB_KEY_LENGTH;
    secp256k1_ec_pubkey_serialize(getCtx(), keybuf, &keybufsize, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    const std::string rawpubkey = std::string((char*)keybuf, EC_PUB_KEY_LENGTH);
    return rawpubkey;
}

std::string PrivKeyToCompressedPubKey(const std::string& rawprivkey) {
    secp256k1_pubkey pubkey;
    const bool res = secp256k1_ec_pubkey_create(getCtx(), &pubkey, (const uint8_t*)rawprivkey.data());
    CHECK_TYPED(res, TypeErrors::PRIVATE_KEY_ERROR, "dont create compressed pubkey");
    uint8_t keybuf[EC_KEY_LENGTH+1];
    size_t keybufsize = EC_KEY_LENGTH+1;
    secp256k1_ec_pubkey_serialize(getCtx(), keybuf, &keybufsize, &pubkey, SECP256K1_EC_COMPRESSED);
    const std::string rawpubkey = std::string((char*)keybuf, keybufsize);
    return rawpubkey;
}

static std::string doubleHash(const std::string &str) {
    uint8_t firsthash[CryptoPP::SHA256::DIGESTSIZE] = {0};
    uint8_t secondhash[CryptoPP::SHA256::DIGESTSIZE] = {0};
    CryptoPP::SHA256 sha256;
    sha256.CalculateDigest(firsthash, (const uint8_t*)str.data(), str.size());
    sha256.CalculateDigest(secondhash, firsthash, CryptoPP::SHA256::DIGESTSIZE);
    return std::string((const char*)secondhash, CryptoPP::SHA256::DIGESTSIZE);
}

std::string PubkeyToAddress(const std::string& rawpubkey, bool testnet) {
    CHECK_TYPED(rawpubkey.size() == EC_PUB_KEY_LENGTH, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect pub key");
    const size_t ADDRESS_LENGTH = 25;
    uint8_t address[ADDRESS_LENGTH] = {0};
    uint8_t* pk = (uint8_t*)rawpubkey.data();
    CHECK_TYPED(pk[0] == 0x04, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "incorrect pub key");
    CryptoPP::SHA256 sha256;
    uint8_t sha256hash[CryptoPP::SHA256::DIGESTSIZE] = {0};
    //Подсчитываем первый sha256-хэш.
    sha256.CalculateDigest(sha256hash, pk, EC_PUB_KEY_LENGTH);
    //Сетевой байт
    if (testnet) {
        address[0] = 0x6F;
    } else {
        address[0] = 0;
    }
    //RIPEMD160-хэш от предыдущего
    CHECK_TYPED(ADDRESS_LENGTH >= CryptoPP::RIPEMD160::DIGESTSIZE + 1, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Ups");
    CryptoPP::RIPEMD160 ripemd;
    ripemd.CalculateDigest(&address[1], sha256hash, CryptoPP::SHA256::DIGESTSIZE);
    const std::string finalhash = doubleHash(std::string((const char*)address, CryptoPP::RIPEMD160::DIGESTSIZE + 1));
    address[21] = finalhash[0];
    address[22] = finalhash[1];
    address[23] = finalhash[2];
    address[24] = finalhash[3];
    const std::string btcaddress = std::string((char*)address, ADDRESS_LENGTH);
    return btcaddress;
}

std::string CompressedPubkeyToAddress(const std::string& rawpubkey, bool testnet) {
    CHECK_TYPED(rawpubkey.size() == EC_KEY_LENGTH+1, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect pub key");
    const size_t ADDRESS_LENGTH = 25;
    uint8_t address[ADDRESS_LENGTH] = {0};
    uint8_t* pk = (uint8_t*)rawpubkey.data();
    CHECK_TYPED(pk[0] == 0x03 || pk[0] == 0x02, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect pub key");
    CryptoPP::SHA256 sha256;
    uint8_t sha256hash[CryptoPP::SHA256::DIGESTSIZE] = {0};
    //Подсчитываем первый sha256-хэш.
    sha256.CalculateDigest(sha256hash, pk, EC_KEY_LENGTH+1);
    //Сетевой байт
    if (testnet) {
        address[0] = 0x6F;
    } else {
        address[0] = 0;
    }
    //RIPEMD160-хэш от предыдущего
    CHECK(ADDRESS_LENGTH >= CryptoPP::RIPEMD160::DIGESTSIZE + 1, "Ups");
    CryptoPP::RIPEMD160 ripemd;
    ripemd.CalculateDigest(&address[1], sha256hash, CryptoPP::SHA256::DIGESTSIZE);
    //Сохраняем чек-сумму
    const std::string finalhash = doubleHash(std::string((const char*)address, CryptoPP::RIPEMD160::DIGESTSIZE + 1));
    address[21] = finalhash[0];
    address[22] = finalhash[1];
    address[23] = finalhash[2];
    address[24] = finalhash[3];
    const std::string btcaddress = std::string((char*)address, ADDRESS_LENGTH);
    return btcaddress;
}

std::string AddressToPubkeyScript(const std::string& address, bool isDecode) {
    std::vector<unsigned char> addr;
    if (isDecode) {
        const bool res = DecodeBase58(address.c_str(), addr);
        CHECK_TYPED(res, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address " + address);
    } else {
        addr.assign(address.begin(), address.end());
    }
    CHECK_TYPED(addr.size() == 25, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address " + address);

    if (addr[0] == 111 || addr[0] == 0) {
        const std::string prefix = HexStringToDump("76a914");
        const std::string suffix = HexStringToDump("88ac");
        const std::string script = prefix + std::string((char*)addr.data()+1, 20) + suffix;
        return script;
    } else if (addr[0] == 5 || addr[0] == 5) {
        const std::string prefix = HexStringToDump("a914");
        const std::string suffix = HexStringToDump("87");
        const std::string script = prefix + std::string((char*)addr.data()+1, 20) + suffix;
        return script;
    } else {
        throwErrTyped(TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address " + EncodeBase58BTC(addr.data(), addr.data() + addr.size()));
    }
}

static std::string privateKeyToWif(const std::string &privateKey, bool isTestnet, bool isCompressed) {
    std::string wif = privateKey;
    //Префикс сети
    if (isTestnet) {
        wif.insert(wif.begin(), 0xEF);
    } else {
        wif.insert(wif.begin(), 0x80);
    }
    //Добавляем байт, указывающий на использование сжатого публичного ключа
    if (isCompressed) {
        wif.insert(wif.end(), 0x01);
    }
    //В качестве контрольной суммы берем первые 4 байта второго хэша
    wif += doubleHash(wif).substr(0, 4);
    return EncodeBase58BTC((const uint8_t*)wif.data(), (const uint8_t*)wif.data()+wif.size());
}

std::string CreateWIF(bool isTestnet, bool isCompressed) {
    CryptoPP::AutoSeededRandomPool prng;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    privateKey.Initialize(prng, CryptoPP::ASN1::secp256k1());
    CHECK_TYPED(privateKey.Validate(prng, 3), TypeErrors::PRIVATE_KEY_ERROR, "dont create wif");

    std::stringstream ss;
    CryptoPP::Integer exp = privateKey.GetPrivateExponent();
    ss << std::hex << exp;
    std::string privateKeyStr = ss.str();
    //Удаляем символ 'h'
    privateKeyStr = privateKeyStr.substr(0, privateKeyStr.size()-1);
    while (privateKeyStr.size() < 32 * 2) {
        privateKeyStr = "0" + privateKeyStr;
    }
    privateKeyStr = HexStringToDump(privateKeyStr);
    return privateKeyToWif(privateKeyStr, isTestnet, isCompressed);
}

bool isAddressBase56(const std::string &address) {
    std::vector<unsigned char> addr;
    const bool res = DecodeBase58(address.c_str(), addr);
    if (!res) {
        return false;
    }
    if (addr.size() != 25) {
        return false;
    }
    const std::string data(addr.begin(), addr.begin() + 21);
    const std::string addrHash = doubleHash(data);
    for (size_t i = 0; i < 4; i++) {
        if (addr.at(21 + i) != (unsigned char)addrHash.at(i)) {
            return false;
        }
    }
    return true;
}

void checkAddressBase56(const std::string &address) {
    CHECK_TYPED(isAddressBase56(address), TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address " + address);
}

std::string getAddress(const std::string &wif, bool &isCompressed, bool isTestnet) {
    const std::string privKey = WIFToPrivkey(wif, isCompressed);
    std::string address;
    if (isCompressed) {
        const std::string pubKey = PrivKeyToCompressedPubKey(privKey);
        address = CompressedPubkeyToAddress(pubKey, isTestnet);
    } else {
        const std::string pubKey = PrivKeyToPubKey(privKey);
        address = PubkeyToAddress(pubKey, isTestnet);
    }
    const std::string addressBase58 = EncodeBase58BTC((const unsigned char*)address.data(), (const unsigned char*)address.data() + address.size());
    return addressBase58;
}

std::string encryptWif(const std::string &wif, const std::string &normalizedPassphraze) {
    bool isCompressed;
    const std::string addressBase58 = getAddress(wif, isCompressed, false);
    const std::string privKey = WIFToPrivkey(wif, isCompressed);
    CHECK_TYPED(privKey.size() == 32, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect private key");

    const unsigned char firstByte = 0x01;
    const unsigned char secondByte = 0x42;
    unsigned char thirdByte = 0xc0;
    thirdByte |= isCompressed ? 0x20 : 0x00;

    const std::string checksumAddress = doubleHash(addressBase58);
    const std::string salt = checksumAddress.substr(0, 4);
    const int n=16384;
    const int r=8;
    const int p=8;
    std::array<uint8_t, 64> derivedkey;

    const int res = libscrypt_scrypt(
        (const uint8_t*)normalizedPassphraze.c_str(), normalizedPassphraze.size(),
        (const uint8_t*)salt.data(), salt.size(),
        n, r, p,
        derivedkey.data(), derivedkey.size()
    );
    CHECK_TYPED(res == 0, TypeErrors::INCORRECT_PASSWORD, "Incorrect password");
    const std::string derivedHalfStr1(derivedkey.begin(), derivedkey.begin() + derivedkey.size() / 2);
    const std::string derivedHalfStr2(derivedkey.begin() + derivedkey.size() / 2, derivedkey.end());

    std::string ciphertext;
    ciphertext.reserve(1000);
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption e;
    e.SetKeyWithIV((const uint8_t*)derivedHalfStr2.data(), derivedHalfStr2.size(), (const unsigned char*)derivedHalfStr1.data(), 16);
    CryptoPP::StringSource s(privKey.substr(0, 16), true,
        new CryptoPP::StreamTransformationFilter(e,
            new CryptoPP::StringSink(ciphertext),
            CryptoPP::StreamTransformationFilter::NO_PADDING
        )
    );
    CHECK_TYPED(ciphertext.size() == 16, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect cbc result");

    std::string ciphertext2;
    ciphertext2.reserve(1000);
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption e2;
    e2.SetKeyWithIV((const uint8_t*)derivedHalfStr2.data(), derivedHalfStr2.size(), (const unsigned char*)derivedHalfStr1.data() + 16, 16);
    CryptoPP::StringSource s2(privKey.substr(16, 16), true,
        new CryptoPP::StreamTransformationFilter(e2,
            new CryptoPP::StringSink(ciphertext2),
            CryptoPP::StreamTransformationFilter::NO_PADDING
        )
    );
    CHECK_TYPED(ciphertext2.size() == 16, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect cbc result");

    std::string result = std::string("") + (char)firstByte + (char)secondByte + (char)thirdByte + salt + ciphertext + ciphertext2;
    CHECK_TYPED(result.size() == 39, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect result");

    const std::string checksumResult = doubleHash(result);
    result += checksumResult.substr(0, 4);

    return EncodeBase58BTC((const unsigned char*)result.data(), (const unsigned char*)result.data() + result.size());
}

std::string decryptWif(const std::string &encryptedWifBase64, const std::string &normalizedPassphraze) {
    CHECK_TYPED(encryptedWifBase64.size() == 58, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encripted wif size " + std::to_string(encryptedWifBase64.size()));
    std::vector<unsigned char> encryptedWifVect;
    const bool res = DecodeBase58(encryptedWifBase64.c_str(), encryptedWifVect);
    CHECK_TYPED(res, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif key");
    const std::string encryptedWif(encryptedWifVect.begin(), encryptedWifVect.end());
    const unsigned char firstByte = encryptedWif[0];
    const unsigned char secondByte = encryptedWif[1];
    const unsigned char thirdByte = encryptedWif[2];
    CHECK_TYPED(firstByte == 0x01 && secondByte == 0x42, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif");
    const bool isCompressed = (thirdByte & 0x20) != 0;

    const std::string salt = encryptedWif.substr(3, 4);
    const std::string encryptedhalf1 = encryptedWif.substr(7, 16);
    const std::string encryptedhalf2 = encryptedWif.substr(23, 16);
    const std::string checksum = encryptedWif.substr(39);
    CHECK_TYPED(checksum == doubleHash(encryptedWif.substr(0, 39)).substr(0, 4), TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif");

    const int n=16384;
    const int r=8;
    const int p=8;
    std::array<uint8_t, 64> derivedkey;

    const int res8 = libscrypt_scrypt(
        (const uint8_t*)normalizedPassphraze.c_str(), normalizedPassphraze.size(),
        (const uint8_t*)salt.data(), salt.size(),
        n, r, p,
        derivedkey.data(), derivedkey.size()
    );
    CHECK_TYPED(res8 == 0, TypeErrors::INCORRECT_PASSWORD, "Incorrect password");
    const std::string derivedHalfStr1(derivedkey.begin(), derivedkey.begin() + derivedkey.size() / 2);
    const std::string derivedHalfStr2(derivedkey.begin() + derivedkey.size() / 2, derivedkey.end());

    std::string ciphertext;
    ciphertext.reserve(1000);
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption e;
    e.SetKeyWithIV((const uint8_t*)derivedHalfStr2.data(), derivedHalfStr2.size(), (const unsigned char*)derivedHalfStr1.data(), 16);
    CryptoPP::StringSource s(encryptedhalf1, true,
        new CryptoPP::StreamTransformationFilter(e,
            new CryptoPP::StringSink(ciphertext),
            CryptoPP::StreamTransformationFilter::NO_PADDING
        )
    );
    CHECK_TYPED(ciphertext.size() == 16, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect cbc operation result");

    std::string ciphertext2;
    ciphertext2.reserve(1000);
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption e2;
    e2.SetKeyWithIV((const uint8_t*)derivedHalfStr2.data(), derivedHalfStr2.size(), (const unsigned char*)derivedHalfStr1.data() + 16, 16);
    CryptoPP::StringSource s2(encryptedhalf2, true,
        new CryptoPP::StreamTransformationFilter(e2,
            new CryptoPP::StringSink(ciphertext2),
            CryptoPP::StreamTransformationFilter::NO_PADDING
        )
    );
    CHECK_TYPED(ciphertext2.size() == 16, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect cbc operation result");

    const std::string privkey = ciphertext + ciphertext2;
    const std::string wif = privateKeyToWif(privkey, false, isCompressed);

    bool tmp;
    const std::string validAddress = getAddress(wif, tmp, false);
    const std::string doubleHashAddress = doubleHash(validAddress);
    CHECK_TYPED(doubleHashAddress.substr(0, salt.size()) == salt, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect private key");
    return wif;
}
