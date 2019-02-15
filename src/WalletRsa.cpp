#include "WalletRsa.h"

#include "check.h"
#include "utils.h"

#include "openssl_wrapper/openssl_wrapper.h"

const static QString FOLDER_RSA_KEYS("rsa/");
const static QString FILE_PRIV_KEY_SUFFIX(".rsa.priv");
const static QString FILE_PUB_KEY_SUFFIX(".rsa.pub");

WalletRsa::WalletRsa(const QString &folder, const std::string &addr)
    : folder(folder)
    , address(addr)
    , publicKey(getPublicRsaKey(folder, addr))
{
    publicKeyRsa = ::getPublicRsa(publicKey);
}

WalletRsa WalletRsa::fromPublicKey(const std::string &publicKey) {
    WalletRsa wallet;
    wallet.publicKey = publicKey;
    wallet.publicKeyRsa = ::getPublicRsa(publicKey);
    return wallet;
}

const std::string& WalletRsa::getPublikKey() const {
    return publicKey;
}

std::string WalletRsa::encrypt(const std::string &message) const {
    CHECK(publicKeyRsa != nullptr, "Publik key not set");
    return ::encrypt(publicKeyRsa, message, publicKey);
}

void WalletRsa::unlock(const std::string &password) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    CHECK(!address.empty(), "Address empty");
    const QString folderKey = makePath(folder, FOLDER_RSA_KEYS);

    const QString fileName = makePath(folderKey, QString::fromStdString(address).toLower() + FILE_PRIV_KEY_SUFFIX);

    const std::string privateKey = readFile(fileName);
    privateKeyRsa = getPrivateRsa(privateKey, password);

    CHECK(validatePublicKey(privateKeyRsa, publicKeyRsa), "Public key damaged");
}

void WalletRsa::createRsaKey(const QString &folder, const std::string &addr, const std::string &password) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = makePath(folder, FOLDER_RSA_KEYS);
    createFolder(folderKey);
    const std::string privateKey = ::createRsaKey(password);

    const QString fileName = makePath(folderKey, QString::fromStdString(addr).toLower() + FILE_PRIV_KEY_SUFFIX);
    writeToFile(fileName, privateKey, true);

    const QString fileNamePub = makePath(folderKey, QString::fromStdString(addr).toLower() + FILE_PUB_KEY_SUFFIX);
    const std::string publicKey = ::getPublic(privateKey, password);
    writeToFile(fileNamePub, publicKey, true);
}

std::string WalletRsa::getPublicRsaKey(const QString &folder, const std::string &addr) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = makePath(folder, FOLDER_RSA_KEYS);

    const QString fileName = makePath(folderKey, QString::fromStdString(addr).toLower() + FILE_PUB_KEY_SUFFIX);
    const std::string publicKeyHex = readFile(fileName);

    return publicKeyHex;
}

std::string WalletRsa::decryptMessage(const std::string &encryptedMessageHex) const {
    CHECK(privateKeyRsa != nullptr, "Wallet not unlock");
    const std::string decryptMsg = decrypt(privateKeyRsa, encryptedMessageHex, publicKey);
    return decryptMsg;
}
