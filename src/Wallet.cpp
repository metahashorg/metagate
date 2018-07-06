#include "Wallet.h"

#include <QDir>

#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <limits>
#include <array>

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

const std::string Wallet::PREFIX_ONE_KEY_MTH = "mth:";
const std::string Wallet::PREFIX_ONE_KEY_TMH = "tmh:";

const static QString FOLDER_RSA_KEYS("rsa/");
const static QString FILE_METAHASH_PRIV_KEY_SUFFIX(".ec.priv");
const static QString FILE_PRIV_KEY_SUFFIX(".rsa.priv");
const static QString FILE_PUB_KEY_SUFFIX(".rsa.pub");

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
    CHECK_TYPED(ttt2.size() == 65 * 2, TypeErrors::DONT_CREATE_KEY, "Ups " + std::to_string(ttt2.size()));

    result = ttt2;
}

static void printPublicKey(const CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey &privateKey) {
    std::string publicKeyStr;
    getPublicKey(privateKey, publicKeyStr);
    LOG << "publicKey: " << publicKeyStr;
}

static std::string doubleSha(const std::string &str) {
    CryptoPP::SHA256 sha256hashAlg2;
    std::string sha256Hash2;
    sha256Hash2.reserve(500);
    CryptoPP::StringSource ss3(str, true, new CryptoPP::HashFilter(sha256hashAlg2, new CryptoPP::StringSink(sha256Hash2)));

    CryptoPP::SHA256 sha256hashAlg3;
    std::string sha256Hash3;
    sha256Hash3.reserve(500);
    CryptoPP::StringSource ss4(sha256Hash2, true, new CryptoPP::HashFilter(sha256hashAlg3, new CryptoPP::StringSink(sha256Hash3)));

    return sha256Hash3;
}

template<typename Integer>
static std::string toLittleEndian(Integer integer) {
    std::array<unsigned char, sizeof(integer)> arr;
    for (size_t i = 0; i < arr.size(); i++) {
        arr[i] = integer % 256;
        integer /= 256;
    }
    return std::string(arr.begin(), arr.end());
}

static std::string packInteger(uint64_t value) {
    if (value <= 249) {
        return toLittleEndian(uint8_t(value));
    } else if (value <= std::numeric_limits<uint16_t>::max()) {
        return toLittleEndian(uint8_t(250)) + toLittleEndian(uint16_t(value));
    } else if (value <= std::numeric_limits<uint32_t>::max()) {
        return toLittleEndian(uint8_t(251)) + toLittleEndian(uint32_t(value));
    } else {
        return toLittleEndian(uint8_t(252)) + toLittleEndian(uint64_t(value));
    }
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

    const std::string sha256Hash3 = doubleSha(ripemdHash);
    ripemdHash += sha256Hash3.substr(0, 4);

    const std::string hexAddr = "0x" + toHex(ripemdHash);

    CHECK_TYPED(hexAddr.size() == 52, TypeErrors::DONT_CREATE_KEY, "Incorrect address");

    //std::cout << hexAddr << std::endl;

    return hexAddr;
}

void Wallet::checkAddress(const std::string &address) {
    std::string addr = address;
    CHECK_TYPED(addr.size() == 52, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address");
    CHECK_TYPED(addr.compare(0, 2, "0x") == 0, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address");
    addr = addr.substr(2);

    const std::string binAddress = fromHex(addr);
    const std::string payload = binAddress.substr(0, binAddress.size() - 4);
    const std::string hash = binAddress.substr(binAddress.size() - 4);

    const std::string doubleHash = doubleSha(payload);
    CHECK_TYPED(doubleHash.substr(0, hash.size()) == hash, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address");
}

void Wallet::createWallet(const QString &folder, const std::string &password, std::string &publicKey, std::string &addr){
    CHECK_TYPED(!password.empty(), TypeErrors::INCORRECT_USER_DATA, "Empty password");

    QDir dir(folder);
    const bool resultCreate = dir.mkpath(folder);
    CHECK_TYPED(resultCreate, TypeErrors::DONT_CREATE_FOLDER, "dont create folder");

    CryptoPP::AutoSeededRandomPool prng;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;

    privateKey.Initialize(prng, CryptoPP::ASN1::secp256r1());
    const bool resultCreatePrivate = privateKey.Validate(prng, 3);
    CHECK_TYPED(resultCreatePrivate, TypeErrors::DONT_CREATE_KEY, "dont create public key");

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
    CHECK_TYPED(!password.empty(), TypeErrors::INCORRECT_USER_DATA, "Empty password");
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
        throwErrTyped(TypeErrors::INCORRECT_PASSWORD, std::string("Dont load private key. Possibly incorrect password. ") + e.what());
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
        throwErrTyped(TypeErrors::DONT_SIGN, std::string("dont sign ") + e.what());
    }
}

bool Wallet::verify(const std::string &message, const std::string &signature, const std::string &publicKey) {
    try {
        std::string signatureBinary = fromHex(signature);

        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicK;
        publicK.AccessGroupParameters().SetEncodeAsOID(true);
        std::string publicKeyStr;
        publicKeyStr.reserve(1000);
        CryptoPP::StringSource sss(publicKey, true, new CryptoPP::HexDecoder());
        publicK.Load(sss);

        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(publicK);

        std::string signature2(128 / 2, 0);
        const size_t resultSize = CryptoPP::DSAConvertSignatureFormat(
            (byte*)signature2.data(), signature2.size(), CryptoPP::DSASignatureFormat::DSA_P1363,
            (const byte*)signatureBinary.data(), signatureBinary.size(), CryptoPP::DSASignatureFormat::DSA_DER
        );
        signature2.resize(resultSize);

        CryptoPP::StringSource ss2(message + signature2, true,
            new CryptoPP::SignatureVerificationFilter(
                verifier, nullptr, CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION
           )
        );

        return true;
    } catch (const std::exception &e) {
        return false;
    }
}

std::string Wallet::genTx(const std::string &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const std::string &data) {
    checkAddress(toAddress);
    std::string result;
    result += fromHex(toAddress.substr(2));
    result += packInteger(value);
    result += packInteger(fee);
    result += packInteger(nonce);
    CHECK_TYPED(data.empty(), TypeErrors::INCORRECT_USER_DATA, "Data not empty");
    result += packInteger(data.size());

    return result;
}

void Wallet::sign(const std::string &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const std::string &data, std::string &txHex, std::string &signature, std::string &publicKey) {
    const std::string txBinary = genTx(toAddress, value, fee, nonce, data);
    signature = sign(txBinary, publicKey);
    txHex = toHex(txBinary);
}

void Wallet::createRsaKey(const QString &folder, const std::string &addr, const std::string &password) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = QDir(folder).filePath(FOLDER_RSA_KEYS);
    const std::string privateKey = ::createRsaKey(password);

    const QString fileName = (QDir(folderKey).filePath(QString::fromStdString(addr).toLower() + FILE_PRIV_KEY_SUFFIX));
    writeToFile(fileName, privateKey, true);

    const QString fileNamePub = (QDir(folderKey).filePath(QString::fromStdString(addr).toLower() + FILE_PUB_KEY_SUFFIX));
    const std::string publicKey = ::getPublic(privateKey, password);
    writeToFile(fileNamePub, publicKey, true);
}

std::string Wallet::getPublicKeyMessage(const QString &folder, const std::string &addr) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = QDir(folder).filePath(FOLDER_RSA_KEYS);

    const QString fileName = (QDir(folderKey).filePath(QString::fromStdString(addr).toLower() + FILE_PUB_KEY_SUFFIX));
    const std::string publicKeyHex = readFile(fileName);

    return publicKeyHex;
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

std::string Wallet::getPrivateKey(const QString &folder, const std::string &addr, bool isCompact, bool isTMH) {
    const QString fullPath = makeFullWalletPath(folder, addr);
    std::string privKey = readFile(fullPath);

    if (isCompact) {
        CHECK_TYPED(privKey.size() > PRIV_KEY_PREFIX.size() + PRIV_KEY_SUFFIX.size(), TypeErrors::PRIVATE_KEY_ERROR, "Incorrect private key");
        CHECK_TYPED(privKey.compare(0, PRIV_KEY_PREFIX.size(), PRIV_KEY_PREFIX) == 0, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect private key");
        privKey = privKey.substr(PRIV_KEY_PREFIX.size());

        const size_t found = privKey.find(PRIV_KEY_SUFFIX);
        CHECK_TYPED(found != privKey.npos, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect private key");
        privKey = privKey.substr(0, found);

        privKey = COMPACT_FORMAT + CURRENT_COMPACT_FORMAT + "\n" + privKey;
    }

    std::string result;
    if (isTMH) {
        result = PREFIX_ONE_KEY_TMH + privKey;
    } else {
        result = PREFIX_ONE_KEY_MTH + privKey;
    }
    return result;
}

void Wallet::savePrivateKey(const QString &folder, const std::string &data, const std::string &password) {
    std::string result;
    if (data.compare(0, PREFIX_ONE_KEY_MTH.size(), PREFIX_ONE_KEY_MTH) == 0) {
        result = data.substr(PREFIX_ONE_KEY_MTH.size());
    } else if (data.compare(0, PREFIX_ONE_KEY_TMH.size(), PREFIX_ONE_KEY_TMH) == 0) {
        result = data.substr(PREFIX_ONE_KEY_TMH.size());
    } else {
        throwErrTyped(TypeErrors::INCORRECT_USER_DATA, "Incorrect data");
    }

    if (result.compare(0, COMPACT_FORMAT.size(), COMPACT_FORMAT) == 0) {
        result = result.substr(COMPACT_FORMAT.size());
        CHECK_TYPED(result.compare(0, CURRENT_COMPACT_FORMAT.size() + 1, CURRENT_COMPACT_FORMAT + "\n") == 0, TypeErrors::INCORRECT_USER_DATA, "Incorrect private key");
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
