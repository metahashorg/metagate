#include "cert.h"
#include "utils2.h"
#include "const.h"

#include "scrypt/libscrypt.h"

#include <cryptopp/oids.h>
#include <cryptopp/keccak.h>
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>

#include <iostream>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"

static CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey LoadPrivateKey(uint8_t* privkey, size_t privkeysize)
{
    CryptoPP::Integer x(privkey, privkeysize);
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    privateKey.Initialize(CryptoPP::ASN1::secp256k1(), x);
    return privateKey;
}

static void ParseCert(const char* certContent, CertParams& params) {
    const QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(std::string(certContent)).toUtf8());
    const QJsonObject root = document.object();

    CHECK(root.contains("address") && root.value("address").isString(), "address field not found in private key");
    params.address = ("0x" + root.value("address").toString()).toStdString();
    CHECK(root.contains("version") && root.value("version").isDouble(), "version field not found in private key");
    params.version = root.value("version").toInt();
    CHECK(root.contains("crypto") && root.value("crypto").isObject(), "crypto field not found in private key");
    const auto &doc = root.value("crypto").toObject();
    CHECK(doc.contains("cipher") && doc.value("cipher").isString(), "cipher field not found in private key");
    CHECK(doc.contains("ciphertext") && doc.value("ciphertext").isString(), "ciphertext field not found in private key");
    CHECK(doc.contains("cipherparams") && doc.value("cipherparams").isObject(), "cipherparams field not found in private key");
    const auto &doc2 = doc.value("cipherparams").toObject();
    CHECK(doc2.contains("iv") && doc2.value("iv").isString(), "iv field not found in private key");
    //Читаем параметры для aes
    params.cipher = doc.value("cipher").toString().toStdString();
    params.ciphertext = doc.value("ciphertext").toString().toStdString();
    params.iv = doc2.value("iv").toString().toStdString();

    CHECK(doc.contains("kdf") && doc.value("kdf").isString(), "kdf field not found in private key");
    CHECK(doc.contains("kdfparams") && doc.value("kdfparams").isObject(), "kdfparams field not found in private key");
    const auto &doc3 = doc.value("kdfparams").toObject();
    CHECK(doc3.contains("dklen") && doc3.value("dklen").isDouble(), "dklen field not found in private key");
    CHECK(doc3.contains("n") && doc3.value("n").isDouble(), "n field not found in private key");
    CHECK(doc3.contains("p") && doc3.value("p").isDouble(), "p field not found in private key");
    CHECK(doc3.contains("r") && doc3.value("r").isDouble(), "r field not found in private key");
    CHECK(doc3.contains("salt") && doc3.value("salt").isString(), "salt field not found in private key");
    //Читаем параметры для kdf
    params.kdftype = doc.value("kdf").toString().toStdString();
    params.dklen = doc3.value("dklen").toInt();
    params.n = doc3.value("n").toInt();
    params.p = doc3.value("p").toInt();
    params.r = doc3.value("r").toInt();
    params.salt = doc3.value("salt").toString().toStdString();
    CHECK(doc.contains("mac") && doc.value("mac").isString(), "mac field not found in private key");
    params.mac = doc.value("mac").toString().toStdString();
}

std::string DeriveAESKeyFromPassword(const std::string& password, CertParams& params)
{
    uint8_t derivedKey[EC_KEY_LENGTH] = {0};
    std::string rawsalt = HexStringToDump(params.salt);
    libscrypt_scrypt((const uint8_t*)password.c_str(), password.size(),
                        (const uint8_t*)rawsalt.c_str(), rawsalt.size(),
                        params.n, params.r, params.p,
                        derivedKey, EC_KEY_LENGTH);
    return std::string((char*)derivedKey, EC_KEY_LENGTH);
}

bool CheckPassword(const std::string& derivedKey, const CertParams& params)
{
    uint8_t hs[EC_KEY_LENGTH];
    CHECK(derivedKey.size() >= 32, "Incorrect derivedKey");
    std::string hashdata = derivedKey.substr(16, 16) + HexStringToDump(params.ciphertext);
    CryptoPP::Keccak k(EC_KEY_LENGTH);
    k.Update((uint8_t*)hashdata.c_str(), hashdata.size());
    k.TruncatedFinal(hs, EC_KEY_LENGTH);
    return (params.mac.compare(DumpToHexString(hs, EC_KEY_LENGTH)) == 0);
}

std::string DecodePrivateKey(const std::string& derivedkey, CertParams& params)
{
    std::string privkey = "";
    std::string rawiv = HexStringToDump(params.iv);
    privkey.reserve(EC_PUB_KEY_LENGTH + 10);
    CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption d;
    d.SetKeyWithIV((const uint8_t*)derivedkey.c_str(), 16, (const uint8_t*)rawiv.c_str());
    CryptoPP::StringSource s(HexStringToDump(params.ciphertext), true,
        new CryptoPP::StreamTransformationFilter(d,
            new CryptoPP::StringSink(privkey)
        )
    );
    return privkey;
}

CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey DecodeCert(const char* certContent, const std::string& pass, uint8_t* rawkey)
{
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey pk;
    CertParams params;
    ParseCert(certContent, params);
    std::string derivedkey = DeriveAESKeyFromPassword(pass, params);
    CHECK(CheckPassword(derivedkey, params), "incorrect password");
    std::string privkey = DecodePrivateKey(derivedkey, params);
    CHECK(privkey.size() >= 32, "Incorrect privkey");
    CHECK(EC_KEY_LENGTH >= 32, "Ups");
    memcpy(rawkey, privkey.c_str(), 32);
    pk = LoadPrivateKey((uint8_t*)privkey.c_str(), privkey.size());
    return pk;
}

std::string getAddressFromFile(const char* certContent) {
    CertParams params;
    ParseCert(certContent, params);
    return params.address;
}
