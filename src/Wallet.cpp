#include "Wallet.h"

#include <QDir>

#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>

#include <cryptopp/rsa.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/osrng.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#include <cryptopp/pem.h>
#include <cryptopp/ripemd.h>

#include "openssl_wrapper/openssl_wrapper.h"

#include "check.h"
#include "Log.h"
#include "utils.h"
#include "TypedException.h"

const static QString FOLDER_RSA_KEYS("rsa/");
const static QString FILE_METAHASH_PRIV_KEY_SUFFIX(".ec.priv");
const static QString FILE_PRIV_KEY_SUFFIX(".rsa.priv");

const std::string PRIV_KEY_PREFIX = "-----BEGIN EC PRIVATE KEY-----\n";
const std::string PRIV_KEY_SUFFIX = "\n-----END EC PRIVATE KEY-----";

const std::string COMPACT_FORMAT = "f:";
const std::string CURRENT_COMPACT_FORMAT = "1";

QString Wallet::makeFullWalletPath(const QString &folder, const std::string &addr) {
    return QDir::cleanPath(QDir(folder).filePath(QString::fromStdString(addr) + FILE_METAHASH_PRIV_KEY_SUFFIX));
}

static void getPublicKey(const CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey &privateKey, std::string &result) {
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
    privateKey.MakePublicKey(publicKey);
    publicKey.AccessGroupParameters().SetEncodeAsOID(true);

    std::string publicKeyStr;
    publicKeyStr.reserve(1000);
    CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(publicKeyStr), true);
    publicKey.DEREncode(encoder);
    encoder.MessageEnd();

    //publicKeyStr.erase(std::remove(publicKeyStr.begin(), publicKeyStr.end(), '\n'), publicKeyStr.end());
    //publicKeyStr = toBase64(publicKeyStr).toStdString();
    result = publicKeyStr;
}

static void getPublicKey2(const CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey &privateKey, std::string &result) {
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
    privateKey.MakePublicKey(publicKey);

    const CryptoPP::ECP::Point& y3 = publicKey.GetPublicElement();
    const CryptoPP::Integer& y3_x = y3.x;
    const CryptoPP::Integer& y3_y = y3.y;

    std::stringstream sst;
    sst << std::hex << y3_x;
    std::string xStr = sst.str();
    xStr.erase(std::remove(xStr.begin(), xStr.end(), 'h'), xStr.end());
    while (xStr.size() < 32 * 2) {
        xStr = "0" + xStr;
    }

    std::stringstream sst2;
    sst2 << std::hex << y3_y;
    std::string yStr = sst2.str();
    yStr.erase(std::remove(yStr.begin(), yStr.end(), 'h'), yStr.end());
    while (yStr.size() < 32 * 2) {
        yStr = "0" + yStr;
    }

    std::string ttt2 = "04" + xStr + yStr;
    CHECK(ttt2.size() == 65 * 2, "Ups " + std::to_string(ttt2.size()));

    result = ttt2;
}

static void printPublicKey(const CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey &privateKey) {
    std::string publicKeyStr;
    getPublicKey(privateKey, publicKeyStr);
    LOG << "publicKey: " << publicKeyStr;
}

std::string Wallet::createAddress(const std::string &publicKeyBinary) {
    CryptoPP::SHA256 sha256hashAlg;
    std::string sha256Hash;
    sha256Hash.reserve(500);
    CryptoPP::StringSource ss1(publicKeyBinary, true, new CryptoPP::HashFilter(sha256hashAlg, new CryptoPP::StringSink(sha256Hash)));

    CryptoPP::RIPEMD160 ripemdHashAlg;
    std::string ripemdHash;
    ripemdHash.reserve(250);
    CryptoPP::StringSource ss2(sha256Hash, true, new CryptoPP::HashFilter(ripemdHashAlg, new CryptoPP::StringSink(ripemdHash)));
    ripemdHash = '\0' + ripemdHash;

    CryptoPP::SHA256 sha256hashAlg2;
    std::string sha256Hash2;
    sha256Hash2.reserve(500);
    CryptoPP::StringSource ss3(ripemdHash, true, new CryptoPP::HashFilter(sha256hashAlg2, new CryptoPP::StringSink(sha256Hash2)));

    CryptoPP::SHA256 sha256hashAlg3;
    std::string sha256Hash3;
    sha256Hash3.reserve(500);
    CryptoPP::StringSource ss4(sha256Hash2, true, new CryptoPP::HashFilter(sha256hashAlg3, new CryptoPP::StringSink(sha256Hash3)));
    ripemdHash += sha256Hash3.substr(0, 4);

    const std::string hexAddr = "0x" + toHex(ripemdHash);

    //std::cout << hexAddr << std::endl;

    return hexAddr;
}

void Wallet::createWallet(const QString &folder, const std::string &password, std::string &publicKey, std::string &addr){
    CHECK(!password.empty(), "Empty password");

    QDir dir(folder);
    const bool resultCreate = dir.mkpath(folder);
    if (!resultCreate) {
        throw TypedException(TypeErrors::DONT_CREATE_FOLDER, "dont create folder");
    }

    CryptoPP::AutoSeededRandomPool prng;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;

    privateKey.Initialize(prng, CryptoPP::ASN1::secp256r1());
    const bool resultCreatePrivate = privateKey.Validate(prng, 3);
    if (!resultCreatePrivate) {
        throw TypedException(TypeErrors::DONT_CREATE_PUBLIC_KEY, "dont create public key");
    }

    std::string pubKey;
    getPublicKey(privateKey, pubKey);
    publicKey = pubKey;
    std::string pubKeyElements;
    getPublicKey2(privateKey, pubKeyElements);
    const std::string pubKeyBinary = fromHex(pubKeyElements);

    const std::string hexAddr = createAddress(pubKeyBinary);

    const QString filePath = makeFullWalletPath(folder, hexAddr);
#ifdef TARGET_WINDOWS
    auto fileNameCStr = filePath.toStdWString();
#else
    auto fileNameCStr = filePath.toStdString();
#endif
    std::ofstream file1(fileNameCStr);
    CryptoPP::FileSink fs(file1);
    CryptoPP::PEM_Save(fs, prng, privateKey, "AES-128-CBC", password.c_str(), password.size());

    addr = hexAddr;
}

std::vector<std::pair<QString, QString>> Wallet::getAllWalletsInFolder(const QString &folder) {
    std::vector<std::pair<QString, QString>> result;

    const QDir dir(folder);
    const QStringList allFiles = dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    for (const QString &file: allFiles) {
        if (file.endsWith(FILE_METAHASH_PRIV_KEY_SUFFIX)) {
            const std::string address = file.split(FILE_METAHASH_PRIV_KEY_SUFFIX).first().toStdString();
            result.emplace_back(QString::fromStdString(address), makeFullWalletPath(folder, address));
        }
    }

    return result;
}

Wallet::Wallet(const QString &folder, const std::string &name, const std::string &password)
    : folder(folder)
    , name(name)
{
    CHECK(!password.empty(), "Empty password");
    fullPath = makeFullWalletPath(folder, name);
    try {
#ifdef TARGET_WINDOWS
        auto fileNameCStr = fullPath.toStdWString();
#else
        auto fileNameCStr = fullPath.toStdString();
#endif
        std::ifstream file1(fileNameCStr);
        CryptoPP::FileSource fs(file1, true /*binary*/);
        CryptoPP::PEM_Load(fs, privateKey, password.c_str(), password.size());
        CryptoPP::AutoSeededRandomPool prng;
        privateKey.Validate(prng, 3);
    } catch (const std::exception &e) {
        throw TypedException(TypeErrors::DONT_LOAD_PRIVATE_KEY, std::string("Dont load private key. Possibly incorrect password. ") + e.what());
    }
}

std::string Wallet::sign(const std::string &message, std::string &publicKey){
    try {
        printPublicKey(privateKey);

        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer signer(privateKey);

        size_t siglen = signer.MaxSignatureLength();
        std::string signature(siglen, 0x00);

        siglen = signer.SignMessage(prng, (const byte*)message.data(), message.size(), (byte*)signature.data());
        signature.resize(siglen);

        std::string signature2(signature.size() * 10, 0);
        const size_t resultSize = CryptoPP::DSAConvertSignatureFormat(
            (byte*)signature2.data(), signature2.size(), CryptoPP::DSASignatureFormat::DSA_DER,
            (const byte*)signature.data(), signature.size(), CryptoPP::DSASignatureFormat::DSA_P1363
        );
        signature2.resize(resultSize);

        std::string pubKey;
        getPublicKey(privateKey, pubKey);
        publicKey = pubKey;

        return toHex(signature2);
    } catch (const std::exception &e) {
        throw TypedException(TypeErrors::DONT_SIGN, std::string("dont sign ") + e.what());
    }
}

std::string Wallet::createRsaKey(const QString &folder, const std::string &addr, const std::string &password) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = QDir(folder).filePath(FOLDER_RSA_KEYS);
    const auto pair = ::createRsaKey(password);

    const QString fileName = (QDir(folderKey).filePath(QString::fromStdString(addr).toLower() + FILE_PRIV_KEY_SUFFIX));
    writeToFile(fileName, pair.first, true);

    return pair.second;
}

std::string Wallet::decryptMessage(const QString &folder, const std::string &addr, const std::string &password, const std::string &encryptedMessageHex) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = QDir(folder).filePath(FOLDER_RSA_KEYS);

    const QString fileName = (QDir(folderKey).filePath(QString::fromStdString(addr).toLower() + FILE_PRIV_KEY_SUFFIX));
    const std::string wifStr = readFile(fileName);

    const std::string privateKey = wifStr;
    const std::string decryptMsg = decrypt(privateKey, password, encryptedMessageHex);
    return decryptMsg;
}

std::string Wallet::encryptMessage(const std::string &publicKeyHex, const std::string &message) {
    return encrypt(publicKeyHex, message);
}

std::string Wallet::getPrivateKey(const QString &folder, const std::string &addr, bool isCompact) {
    const QString fullPath = makeFullWalletPath(folder, addr);
    std::string privKey = readFile(fullPath);

    if (isCompact) {
        CHECK(privKey.size() > PRIV_KEY_PREFIX.size() + PRIV_KEY_SUFFIX.size(), "Incorrect private key");
        if (privKey.compare(0, PRIV_KEY_PREFIX.size(), PRIV_KEY_PREFIX) == 0) {
            privKey = privKey.substr(PRIV_KEY_PREFIX.size());
        } else {
            throwErr("Incorrect private key");
        }

        const size_t found = privKey.find(PRIV_KEY_SUFFIX);
        CHECK(found != privKey.npos, "Incorrect private key");
        privKey = privKey.substr(0, found);

        privKey = COMPACT_FORMAT + CURRENT_COMPACT_FORMAT + "\n" + privKey;
    }
    return privKey;
}

void Wallet::savePrivateKey(const QString &folder, const std::string &data, const std::string &password) {
    std::string result = data;
    if (result.compare(0, COMPACT_FORMAT.size(), COMPACT_FORMAT) == 0) {
        result = result.substr(COMPACT_FORMAT.size());
        CHECK(result.compare(0, CURRENT_COMPACT_FORMAT.size() + 1, CURRENT_COMPACT_FORMAT + "\n") == 0, "Incorrect private key");
        result = result.substr(CURRENT_COMPACT_FORMAT.size() + 1);

        const size_t found = result.find(PRIV_KEY_SUFFIX);
        if (found == result.npos) {
            result += PRIV_KEY_SUFFIX + "\n\n";
        }
        if (result.compare(0, PRIV_KEY_PREFIX.size(), PRIV_KEY_PREFIX) != 0) {
            result = PRIV_KEY_PREFIX + result;
        }
    }

    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    CryptoPP::StringSource fs(result, true /*binary*/);
    CryptoPP::PEM_Load(fs, privateKey, password.c_str(), password.size());
    CryptoPP::AutoSeededRandomPool prng;
    privateKey.Validate(prng, 3);

    std::string pubKeyElements;
    getPublicKey2(privateKey, pubKeyElements);
    const std::string pubKeyBinary = fromHex(pubKeyElements);
    const std::string hexAddr = createAddress(pubKeyBinary);

    const QString filePath = makeFullWalletPath(folder, hexAddr);
    writeToFile(filePath, result, true);
}
