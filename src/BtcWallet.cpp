#include "BtcWallet.h"

#include "btctx/wif.h"
#include "ethtx/utils2.h"
#include "btctx/Base58.h"
#include "btctx/btctx.h"

#include "check.h"
#include "utils.h"
#include "Log.h"

#include <algorithm>
#include <iostream>

#include <QDir>

const static std::string WIF_AND_ADDRESS_DELIMITER = " ";

static QString convertAddressToFileName(const std::string &address) {
    return QString::fromStdString(address.substr(0, address.size() - 3) + "---").toLower();
}

QString BtcWallet::getFullPath(const QString &folder, const std::string &address) {
    QString pathToFile = QDir(folder).filePath(QString::fromStdString(address).toLower());
    if (!QFile(pathToFile).exists()) {
        pathToFile = QDir(folder).filePath(convertAddressToFileName(address));
    }
    return pathToFile;
}

static std::pair<std::string, std::string> getWifAndAddress(const std::string &data, bool isNoEncrypted) {
    const size_t foundDelimiter = data.find(WIF_AND_ADDRESS_DELIMITER);
    std::string wif;
    std::string address;
    if (foundDelimiter == data.npos) {
        wif = data;
        if (isNoEncrypted) {
            bool tmp;
            address = ::getAddress(wif, tmp, false);
        }
    } else {
        wif = data.substr(0, foundDelimiter);
        address = data.substr(foundDelimiter + 1);
    }

    return std::make_pair(wif, address);
}

static std::pair<std::string, std::string> getWifAndAddress(const QString &folder, const std::string &addr, bool isNoEncrypted) {
    const QString pathToFile = BtcWallet::getFullPath(folder, addr);
    const std::string wifAndAddress = readFile(pathToFile);
    return getWifAndAddress(wifAndAddress, isNoEncrypted);
}

std::pair<std::string, std::string> BtcWallet::genPrivateKey(const QString &folder, const QString &password) {
    const bool isCompressed = true;
    const bool isTestnet = false;
    std::string wif = CreateWIF(isTestnet, isCompressed);
    bool tmp;
    const std::string addressBase58 = ::getAddress(wif, tmp, isTestnet);
    CHECK(isCompressed == tmp, "ups");
    if (!password.isNull() && !password.isEmpty()) {
        wif = encryptWif(wif, password.normalized(QString::NormalizationForm_C).toStdString());
        CHECK(wif.substr(0, 2) == "6P", "Incorrect encrypted wif " + wif);
    } else {
        CHECK(wif.substr(0, 2) != "6P", "Incorrect encrypted wif " + wif);
    }

    const QString fileName = QDir(folder).filePath(convertAddressToFileName(addressBase58));
    writeToFile(fileName, wif + WIF_AND_ADDRESS_DELIMITER + addressBase58, true);

    return std::make_pair(addressBase58, wif);
}

BtcWallet::BtcWallet(const QString &folder, const std::string &address_, const QString &password) {
    const auto pair = getWifAndAddress(folder, address_, false);
    const std::string wifEncrypted = pair.first;
    address = pair.second;

    wif = wifEncrypted;
    if (!password.isNull() && !password.isEmpty()) {
        if (wifEncrypted.substr(0, 2) == "6P") {
            wif = decryptWif(wifEncrypted, password.normalized(QString::NormalizationForm_C).toStdString());
        }
    } else {
        CHECK(wifEncrypted.substr(0, 2) != "6P", "Incorrect encrypted wif " + wifEncrypted);
    }

    if (address.empty()) {
        bool tmp;
        address = ::getAddress(wif, tmp, false);
    }
}

BtcWallet::BtcWallet(const std::string &decryptedWif)
    : wif(decryptedWif)
{
    CHECK(decryptedWif.substr(0, 2) != "6P", "Incorrect encrypted wif " + decryptedWif);
}

const std::string& BtcWallet::getAddress() const {
    return address;
}

std::string BtcWallet::genTransaction(const std::vector<BtcInput> &inputs, uint64_t transferAmount, uint64_t fee, const std::string &receiveAddress, bool isTestnet) {
    checkAddressBase56(receiveAddress);

    std::vector<Input> inputs2;
    for (const BtcInput &input: inputs) {
        Input input2;
        input2.wif = wif;
        input2.outBalance = input.outBalance;
        input2.scriptPubkey = HexStringToDump(input.scriptPubkey);
        input2.spendoutnum = input.spendoutnum;
        input2.spendtxid = HexStringToDump(input.spendtxid);

        inputs2.push_back(input2);
    }

    const std::string tx = BuildBTCTransaction(inputs2, fee, transferAmount, receiveAddress, isTestnet);
    return DumpToHexString(tx);
}

static size_t calcSizeTransaction(const std::string& transaction) {
    return transaction.size() / 2;
}

/**
 * Пытается набрать как можно меньше элементов на заданную сумму
 * Может вернуть меньшую сумму, если элементов не достаточно.
 */
template<class Element, typename Value>
static std::vector<Element> greedyAlg(const std::vector<Element> &elements, const Value &allValue) {
    std::vector<Element> sortedVect(elements.begin(), elements.end());
    std::sort(sortedVect.begin(), sortedVect.end());
    std::vector<Element> result;
    Value currValue = 0;
    while (currValue < allValue && !sortedVect.empty()) {
        auto foundIter = std::upper_bound(sortedVect.begin(), sortedVect.end(), Element(allValue - currValue));
        auto foundIter2 = foundIter - 1;
        if ((Value)foundIter2->outBalance == allValue - currValue) {
            foundIter = foundIter2;
        }
        if (foundIter == sortedVect.end()) {
            foundIter--;
        }
        currValue += foundIter->outBalance;
        result.emplace_back(*foundIter);
        sortedVect.erase(foundIter);
    }

    return result;
}

std::string BtcWallet::encode(
    bool allMoney, const int64_t &value, const int64_t &fees,
    const std::string &toAddress,
    const std::vector<BtcInput> &utxos
) {
    LOG << "Utxos size " + std::to_string(utxos.size());

    std::vector<BtcInput> newUtxos;
    if (!allMoney) {
        const int64_t allValue = value + fees;
        newUtxos = greedyAlg(utxos, allValue);
        LOG << "Utxos size2 " + std::to_string(newUtxos.size());
    } else {
        newUtxos = utxos;
    }

    int64_t allUtxoValue = 0;
    for (const BtcInput &utxo: newUtxos) {
        allUtxoValue += utxo.outBalance;
    }

    CHECK(allUtxoValue >= value + fees, "Not enough money. Balance " + std::to_string(allUtxoValue) + ". Value to send " + std::to_string(value) + ". Fees " + std::to_string(fees));

    int64_t feesValue = fees;
    int64_t valueToSend;
    if (allMoney) {
        valueToSend = allUtxoValue - feesValue;
    } else {
        valueToSend = value;
    }

    CHECK(valueToSend > 0, "Not enough money. Balance " + std::to_string(allUtxoValue) + ". Value to send " + std::to_string(value) + ". Fees " + std::to_string(fees));

    const std::string encodedTransaction = genTransaction(newUtxos, valueToSend, feesValue, toAddress, false);

    return encodedTransaction;
}

std::string BtcWallet::buildTransaction(
    const std::vector<BtcInput> &utxos,
    size_t estimateComissionInSatoshi,
    const std::string &valueStr,
    const std::string &feesStr,
    const std::string &receiveAddress
) {
    bool allMoney = false;
    int64_t value = 0;
    if (valueStr == "all") {
        allMoney = true;
        value = 0;
    } else {
        CHECK(isDecimal(valueStr), "Not hex number value");
        allMoney = false;
        value = std::stoll(valueStr);
    }

    int64_t feesEstimate = 0;
    int64_t fees = 0;
    if (feesStr != "auto") {
        CHECK(isDecimal(feesStr), "Not hex number fees");
        fees = std::stoll(feesStr);
    } else {
        CHECK(estimateComissionInSatoshi > 0, "Uncnown estimate comission " + std::to_string(estimateComissionInSatoshi));
        feesEstimate = estimateComissionInSatoshi;
        LOG << "estimated fees1 " + std::to_string(feesEstimate);
    }
    int maxIterations = 10;
    std::string oldTransaction;
    while (true) {
        const size_t oldTransactionSize = calcSizeTransaction(oldTransaction);
        if (feesStr == "auto") {
            fees = (feesEstimate * oldTransactionSize) / 1024;
        }
        if (fees < (int64_t)oldTransactionSize + 30) {
            fees = oldTransactionSize + 30;
        }

        const std::string tmpTransaction = encode(allMoney, value, fees, receiveAddress, utxos);
        if (tmpTransaction.empty()) {
            break;
        }

        oldTransaction = tmpTransaction;

        if (std::abs((int)calcSizeTransaction(tmpTransaction) - (int)oldTransactionSize) <= 30) { // Проверяем, что размер транзакции не изменился после рассчета fees-а.
            break;
        }

        maxIterations--;
        if (maxIterations <= 0) {
            throwErr("I can not estimate fees");
        }
    }
    LOG << "estimated fees2 " + std::to_string(feesEstimate);

    const std::string &encodedTransaction = oldTransaction;

    LOG << "transaction size " + std::to_string(calcSizeTransaction(encodedTransaction));
    //LOGDEBUG << encodedTransaction;

    CHECK(!encodedTransaction.empty(), "Not encode transactions");

    return encodedTransaction;
}

std::vector<std::pair<QString, QString>> BtcWallet::getAllWalletsInFolder(const QString &folder) {
    std::vector<std::pair<QString, QString>> result;

    const QDir dir(folder);
    const QStringList allFiles = dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    for (const QString &file: allFiles) {
        const std::string address = getWifAndAddress(folder, file.toStdString(), true).second;
        CHECK(!address.empty(), "empty result");
        result.emplace_back(QString::fromStdString(address), getFullPath(folder, address));
    }

    return result;
}

std::string BtcWallet::getOneKey(const QString &folder, const std::string &address) {
    const QString filePath = getFullPath(folder, address);
    return readFile(filePath);
}

void BtcWallet::savePrivateKey(const QString &folder, const std::string &data, const QString &password) {
    const std::string addressBase58 = getWifAndAddress(data, true).second;
    const QString pathToFile = QDir(folder).filePath(convertAddressToFileName(addressBase58));
    writeToFile(pathToFile, data, true);
    BtcWallet wallet(folder, addressBase58, password); // Проверяем пароль
}
