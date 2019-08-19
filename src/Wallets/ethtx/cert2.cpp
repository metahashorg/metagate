#include "cert.h"

#include <stdlib.h>

#include <iostream>

#include <secp256k1/include/secp256k1_recovery.h>

#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/keccak.h>
#include <cryptopp/ccm.h>

#include "crossguid/Guid.hpp"

#include "utils2.h"
#include "const.h"

#include "scrypt/libscrypt.h"

#include "check.h"

secp256k1_context const* getCtx();

#ifdef _WIN32
int libscrypt_salt_gen(uint8_t *salt, size_t len);
#else
extern "C" int libscrypt_salt_gen(uint8_t *salt, size_t len);
#endif

std::string keccak(const std::string &data) {
    uint8_t hs[EC_KEY_LENGTH];
    CryptoPP::Keccak k(EC_KEY_LENGTH);
    k.Update((uint8_t*)data.c_str(), data.size());
    k.TruncatedFinal(hs, EC_KEY_LENGTH);
    return std::string(hs, hs + EC_KEY_LENGTH);
}

//Кодирование адреса ethereum
std::string MixedCaseEncoding(const std::string& binaryAddress)
{
    //Берем 16-ричное представление адреса
    std::string hexaddress = DumpToHexString(binaryAddress);
    //Находим keccak256 от него
    const std::string hs = keccak(hexaddress);
    //Переводим хэш в 16-ричное представление
    std::string hexhs = DumpToHexString((const uint8_t*)hs.data(), hs.size());
    //Преобразуем исходный адрес в соответсвии с значением keccak256
    char a = 0;
    std::string normalizedAddr = "";
    CHECK(hexhs.size() >= hexaddress.size(), "Ups");
    for (size_t i = 0; i < hexaddress.size(); i++)
    {
        a = hexhs.at(i);
        if (a >= '0' && a <= '9')
            a = a - '0';
        else
        {
            if (a >= 'a' && a <= 'f')
                a = a - 'a' + 10;
        }
        if (a >= 8)
            normalizedAddr += toupper(hexaddress.at(i));
        else
            normalizedAddr += hexaddress.at(i);
    }
    return normalizedAddr;
}

std::string DeriveAESKeyFromPasswordDefault(const std::string& password, std::string& newsalt)
{
    std::string aeskey = "";
    uint8_t derivedKey[EC_KEY_LENGTH] = {0};
    //Стандартные значения параметров наследования aes-ключа
    uint64_t N = SCRYPT_DEFAULT_N;//Можно выбрать и другое
    uint32_t r = SCRYPT_DEFAULT_r;
    uint32_t p = SCRYPT_DEFAULT_p;
    //Генерируем произвольную "соль"
    uint8_t salt[EC_KEY_LENGTH] = {0};
    if (password.empty())
        return "";
    libscrypt_salt_gen(salt, EC_KEY_LENGTH);
    libscrypt_scrypt((const uint8_t*)password.c_str(), password.size(),
                        salt, EC_KEY_LENGTH,
                        N, r, p,
                        derivedKey, EC_KEY_LENGTH);
    newsalt = std::string((char*)salt, EC_KEY_LENGTH);
    return std::string((char*)derivedKey, EC_KEY_LENGTH);
}

//Создание ключевой пары.
std::string AddressFromPrivateKey(const std::string& privkey)
{
    uint8_t hs[EC_KEY_LENGTH] = {0};
    CryptoPP::AutoSeededRandomPool prng;
    std::string address = "";
    size_t i = 0;
    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_create(getCtx(), &pubkey, (const uint8_t*)privkey.data()))
    {
        //Переворачиваем байты в подписи
        uint8_t* pubkeybuf = (uint8_t*)&pubkey.data;
        uint8_t pubkeybufnew[64] = {0};
        for (i = 0; i < EC_KEY_LENGTH; ++i)
            pubkeybufnew[EC_KEY_LENGTH-1-i] = pubkeybuf[i];
        for (i = 0; i < EC_KEY_LENGTH; ++i)
            pubkeybufnew[EC_KEY_LENGTH*2-1-i] = pubkeybuf[EC_KEY_LENGTH+i];
        //Считаем хэш от ключа
        CryptoPP::Keccak k(EC_KEY_LENGTH);
        k.Update((uint8_t*)&pubkeybufnew, EC_PUB_KEY_LENGTH-1);
        k.TruncatedFinal(hs, EC_KEY_LENGTH);
        //Берем последние 20 байт в качестве адреса
        address = std::string((char*)hs+12, 20);
    }

    return address;
}

std::string CreateRawECDSAKey()
{
    std::string hexkey = "";
    std::stringstream ss;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privkey;
    CryptoPP::AutoSeededRandomPool prng;
    privkey.Initialize(prng, CryptoPP::ASN1::secp256k1());
    if (privkey.Validate(prng, 3))
    {
        ss << std::hex << privkey.GetPrivateExponent();
        hexkey = ss.str();
        hexkey = hexkey.substr(0, hexkey.size()-1);
        while (hexkey.size() < 32 * 2) {
            hexkey = "0" + hexkey;
        }
    }
    if (!hexkey.empty())
        return HexStringToDump(hexkey);
    else
        return "";
}

std::string CreateKeyFile(const CertParams& certparams)
{
    std::string uuid = xg::newGuid();
    std::string keyfile =
        "{\"address\": \"" + DumpToHexString((const uint8_t*)certparams.address.data(), certparams.address.size()) + "\","
            "\"crypto\": {"
                    "\"cipher\": \"aes-128-ctr\","
                    "\"ciphertext\": \"" + DumpToHexString((const uint8_t*)certparams.ciphertext.data(), certparams.ciphertext.size())  + "\","
                    "\"cipherparams\": {"
                                        "\"iv\": \"" + DumpToHexString((const uint8_t*)certparams.iv.data(), certparams.iv.size()) + "\""
                                    "},"
                                "\"kdf\": \"scrypt\","
                                "\"kdfparams\": {"
                                    "\"dklen\": " + std::to_string(certparams.dklen) + ","
                                    "\"n\": " + std::to_string(certparams.n) + ","
                                    "\"p\": " + std::to_string(certparams.p) + ","
                                    "\"r\": " + std::to_string(certparams.r) + ","
                                    "\"salt\": \"" + DumpToHexString((const uint8_t*)certparams.salt.data(), certparams.salt.size()) + "\""
                                "},"
                                "\"mac\": \"" + DumpToHexString((const uint8_t*)certparams.mac.data(), certparams.mac.size()) + "\""
                            "},"
                    "\"id\": \"" + uuid + "\","
                    "\"version\": 3}";
    return keyfile;
}

std::pair<std::string, std::string> EncodePrivKey(const std::string& privkey, const std::string& password)
{
    CertParams certparams;
    std::string jsonkey = "";
    std::string newsalt = "";
    uint8_t iv[EC_KEY_LENGTH/2] = {0};
    uint8_t hsmac[EC_KEY_LENGTH] = {0};
    std::string ciphertext = "";
    ciphertext.reserve(1000);
    if (!privkey.empty() && !password.empty())
    {
        std::string derivedkey = DeriveAESKeyFromPasswordDefault(password, newsalt);
        if (!derivedkey.empty())
        {
            //Создаем произвольный вектор инициализации
            libscrypt_salt_gen(iv, EC_KEY_LENGTH/2);
            //Шифруем
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption e;
            e.SetKeyWithIV((const uint8_t*)derivedkey.c_str(), EC_KEY_LENGTH/2, iv, EC_KEY_LENGTH/2);
            CryptoPP::StringSource s(privkey, true,
                new CryptoPP::StreamTransformationFilter(e,
                    new CryptoPP::StringSink(ciphertext)
                )
            );
            //Считаем mac
            std::string hashdata = derivedkey.substr(16, 16) + ciphertext;
            CryptoPP::Keccak k(EC_KEY_LENGTH);
            k.Update((uint8_t*)hashdata.c_str(), hashdata.size());
            k.TruncatedFinal(hsmac, EC_KEY_LENGTH);
            //Заполняем структуру
            certparams.ciphertext = ciphertext;
            certparams.iv = std::string((char*)iv, EC_KEY_LENGTH/2);
            certparams.dklen = EC_KEY_LENGTH;
            certparams.n = SCRYPT_DEFAULT_N;
            certparams.p = SCRYPT_DEFAULT_p;
            certparams.r = SCRYPT_DEFAULT_r;
            certparams.salt = newsalt;
            certparams.mac = std::string((char*)hsmac, EC_KEY_LENGTH);
            certparams.address = AddressFromPrivateKey(privkey);
            //Собираем файл ключа
            jsonkey = CreateKeyFile(certparams);
        }
    }
    return std::make_pair("0x" + MixedCaseEncoding(certparams.address), jsonkey);
}

std::pair<std::string, std::string> CreateNewKey(const std::string& password) {
    std::string rawprivkey = CreateRawECDSAKey();
    CHECK(!rawprivkey.empty(), "rawprivkey empty");
    return EncodePrivKey(rawprivkey, password);
}
