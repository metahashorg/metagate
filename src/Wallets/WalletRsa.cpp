#include "WalletRsa.h"

#include "check.h"
#include "utilites/utils.h"

#include "Wallet.h"

#include "openssl_wrapper/openssl_wrapper.h"

const static QString FOLDER_RSA_KEYS("rsa/");
const static QString FILE_PRIV_KEY_SUFFIX(".rsa.priv");
const static QString FILE_PUB_KEY_SUFFIX(".rsa.pub");

WalletRsa::WalletRsa(const QString &folder, bool isMhc, const std::string &addr)
    : folder(genFolderRsa(folder, isMhc))
    , address(addr)
    , publicKey(getPublicRsaKey(folder, isMhc, addr))
{
    publicKeyRsa = ::getPublicRsa(publicKey);
}

QString WalletRsa::genFolderRsa(const QString &folder, bool isMhc) {
    return makePath(folder, Wallet::chooseSubfolder(isMhc), FOLDER_RSA_KEYS);
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

bool WalletRsa::validateKeyName(const QString &privKey, const QString &pubkey, const QString &address) {
    CHECK(!address.isEmpty(), "Address empty");
    if (!privKey.endsWith(FILE_PRIV_KEY_SUFFIX) || !pubkey.endsWith(FILE_PUB_KEY_SUFFIX)) {
        return false;
    }
    if (privKey.size() - privKey.lastIndexOf(address) != address.size() + FILE_PRIV_KEY_SUFFIX.size()) {
        return false;
    }
    if (pubkey.size() - pubkey.lastIndexOf(address) != address.size() + FILE_PUB_KEY_SUFFIX.size()) {
        return false;
    }
    return true;
}

std::vector<QString> WalletRsa::getPathsKeys(const QString &folder, bool isMhc, const QString &address) {
    const QString path = genFolderRsa(folder, isMhc);
    const std::vector<QString> result = {path + address + FILE_PRIV_KEY_SUFFIX, path + address + FILE_PUB_KEY_SUFFIX};
    return result;
}

std::string WalletRsa::encrypt(const std::string &message) const {
    CHECK(publicKeyRsa != nullptr, "Publik key not set");
    return ::encrypt(publicKeyRsa, message, publicKey);
}

void WalletRsa::unlock(const std::string &password) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    CHECK(!address.empty(), "Address empty");

    const QString fileName = makePath(folder, QString::fromStdString(address).toLower() + FILE_PRIV_KEY_SUFFIX);

    const std::string privateKey = readFile(fileName);
    privateKeyRsa = getPrivateRsa(privateKey, password);

    CHECK(validatePublicKey(privateKeyRsa, publicKeyRsa), "Public key damaged");
}

void WalletRsa::createRsaKey(const QString &folder, bool isMhc, const std::string &addr, const std::string &password) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = genFolderRsa(folder, isMhc);
    createFolder(folderKey);
    const std::string privateKey = ::createRsaKey(password);

    const QString fileName = makePath(folderKey, QString::fromStdString(addr).toLower() + FILE_PRIV_KEY_SUFFIX);
    writeToFile(fileName, privateKey, true);

    const QString fileNamePub = makePath(folderKey, QString::fromStdString(addr).toLower() + FILE_PUB_KEY_SUFFIX);
    const std::string publicKey = ::getPublic(privateKey, password);
    writeToFile(fileNamePub, publicKey, true);
}

std::string WalletRsa::getPublicRsaKey(const QString &folder, bool isMhc, const std::string &addr) {
    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString folderKey = genFolderRsa(folder, isMhc);

    const QString fileName = makePath(folderKey, QString::fromStdString(addr).toLower() + FILE_PUB_KEY_SUFFIX);
    const std::string publicKeyHex = readFile(fileName);

    return publicKeyHex;
}

std::string WalletRsa::decryptMessage(const std::string &encryptedMessageHex) const {
    CHECK(privateKeyRsa != nullptr, "Wallet not unlock");
    const std::string decryptMsg = decrypt(privateKeyRsa, encryptedMessageHex, publicKey);
    return decryptMsg;
}
