#include "EthWallet.h"

#include <stdlib.h>
#include <string>
#include <vector>

#include <cryptopp/keccak.h>
#include <cryptopp/eccrypto.h>

#include "ethtx/ethtx.h"
#include "ethtx/utils2.h"
#include "ethtx/const.h"
#include "ethtx/cert.h"
#include "ethtx/scrypt/libscrypt.h"

#include "utils.h"
#include "check.h"

#include <iostream>

#include <QDir>

const std::string EthWallet::PREFIX_ONE_KEY = "eth:";

QString EthWallet::getFullPath(const QString &folder, const std::string &address) {
    return QDir(folder).filePath(QString::fromStdString(address).toLower());
}

EthWallet::EthWallet(
    const QString &folder,
    const std::string &address,
    std::string password
) {
    CHECK(!password.empty(), "Empty password");
    const QString pathToFile = getFullPath(folder, address);
    const std::string certcontent = readFile(pathToFile);
    CHECK(!certcontent.empty(), "private file empty");
    rawprivkey.resize(EC_KEY_LENGTH);
    DecodeCert(certcontent.c_str(), password, rawprivkey.data());
}

std::string EthWallet::SignTransaction(
    std::string nonce,
    std::string gasPrice,
    std::string gasLimit,
    std::string to,
    std::string value,
    std::string data
) {
    const std::string transaction = ::SignTransaction(std::string((char*)rawprivkey.data(), rawprivkey.size()), nonce, gasPrice, gasLimit, to, value, data);
    return transaction;
}

std::string EthWallet::genPrivateKey(const QString &folder, const std::string &password) {
    CHECK(!password.empty(), "Empty password");
    const auto pair = CreateNewKey(password);
    const std::string &address = pair.first;
    const std::string &keyValue = pair.second;

    CHECK(!folder.isNull() && !folder.isEmpty(), "Incorrect path to wallet: empty");
    const QString fileName = getFullPath(folder, address);
    writeToFile(fileName, keyValue, true);

    return address;
}

std::vector<std::pair<QString, QString>> EthWallet::getAllWalletsInFolder(const QString &folder) {
    std::vector<std::pair<QString, QString>> result;

    const QDir dir(folder);
    const QStringList allFiles = dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    for (const QString &file: allFiles) {
        const std::string fileName = file.toStdString();
        if (fileName.substr(0, 2) == "0x") {
            const std::string addressPart = fileName.substr(2);
            const std::string address = "0x" + MixedCaseEncoding(HexStringToDump(addressPart));
            result.emplace_back(QString::fromStdString(address), getFullPath(folder, address));
        }
    }

    return result;
}

std::string EthWallet::makeErc20Data(const std::string &valueHex, const std::string &address) {
    std::string result = "0xa9059cbb";

    CHECK(address.substr(0, 2) == "0x", "Incorrect address " + address);
    CHECK(address.size() == 42, "Incorrect address " + address);
    CHECK(valueHex.substr(0, 2) == "0x", "Incorrect address " + valueHex);

    std::string param1 = address.substr(2);
    param1.insert(param1.begin(), 64 - param1.size(), '0');

    std::string param2 = valueHex.substr(2);
    param2.insert(param2.begin(), 64 - param2.size(), '0');

    return result + param1 + param2;
}

std::string EthWallet::getOneKey(const QString &folder, const std::string &address) {
    const QString pathToFile = getFullPath(folder, address);
    return PREFIX_ONE_KEY + readFile(pathToFile);
}

void EthWallet::savePrivateKey(const QString &folder, const std::string &data, const std::string &password) {
    CHECK(data.compare(0, PREFIX_ONE_KEY.size(), PREFIX_ONE_KEY) == 0, "Incorrect data");

    const std::string content = data.substr(PREFIX_ONE_KEY.size());

    std::vector<uint8_t> tmp(1000, 0);
    DecodeCert(content.c_str(), password, tmp.data()); // Проверяем пароль
    const std::string address = getAddressFromFile(content.c_str());

    const QString pathToFile = getFullPath(folder, address);
    writeToFile(pathToFile, content, true);
}

void EthWallet::checkAddress(const std::string &address) {
    CHECK(address.size() == 42, "Incorrect address");
    CHECK(address.compare(0, 2, "0x") == 0, "Incorrect address");

    const std::string addressPart = address.substr(2);

    const std::string addressMixed = "0x" + MixedCaseEncoding(HexStringToDump(addressPart));
    CHECK(addressMixed == address, "Incorrect Address");
}
