#include "Wallets.h"

#include <QStandardPaths>

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "Log.h"
#include "check.h"

#include "utilites/utils.h"
#include "utilites/unzip.h"

#include "Wallet.h"
#include "BtcWallet.h"
#include "EthWallet.h"
#include "WalletRsa.h"

#include "Paths.h"

#include "auth/Auth.h"

#include "transactions/Transactions.h"

#include "Utils/UtilsManager.h"

#include "GetActualWalletsEvent.h"

SET_LOG_NAMESPACE("WLTS");

namespace wallets {

Wallets::Wallets(auth::Auth &auth, utils::Utils &utils, QObject *parent)
    : TimerClass(5s, parent)
    , walletDefaultPath(getWalletPath())
    , utils(utils)
{
    Q_CONNECT(&auth, &auth::Auth::logined2, this, &Wallets::onLogined);
    Q_CONNECT(&fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &Wallets::onDirChanged);

    Q_CONNECT(this, &Wallets::getListWallets, this, &Wallets::onGetListWallets);
    Q_CONNECT(this, &Wallets::getListWallets2, this, &Wallets::onGetListWallets2);
    Q_CONNECT(this, &Wallets::createWatchWalletsList, this, &Wallets::onCreateWatchWalletsList);
    Q_CONNECT(this, &Wallets::createWallet, this, &Wallets::onCreateWallet);
    Q_CONNECT(this, &Wallets::createWatchWallet, this, &Wallets::onCreateWatchWallet);
    Q_CONNECT(this, &Wallets::removeWatchWallet, this, &Wallets::onRemoveWatchWallet);
    Q_CONNECT(this, &Wallets::checkWalletExist, this, &Wallets::onCheckWalletExist);
    Q_CONNECT(this, &Wallets::checkWalletPassword, this, &Wallets::onCheckWalletPassword);
    Q_CONNECT(this, &Wallets::checkAddress, this, &Wallets::onCheckAddress);
    Q_CONNECT(this, &Wallets::createContractAddress, this, &Wallets::onCreateContractAddress);
    Q_CONNECT(this, &Wallets::signMessage, this, &Wallets::onSignMessage);
    Q_CONNECT(this, &Wallets::signMessage2, this, &Wallets::onSignMessage2);
    Q_CONNECT(this, &Wallets::signAndSendMessage, this, &Wallets::onSignAndSendMessage);
    Q_CONNECT(this, &Wallets::signAndSendMessageDelegate, this, &Wallets::onSignAndSendMessageDelegate);
    Q_CONNECT(this, &Wallets::getOnePrivateKey, this, &Wallets::onGetOnePrivateKey);
    Q_CONNECT(this, &Wallets::savePrivateKey, this, &Wallets::onSavePrivateKey);
    Q_CONNECT(this, &Wallets::saveRawPrivateKey, this, &Wallets::onSaveRawPrivateKey);
    Q_CONNECT(this, &Wallets::getRawPrivateKey, this, &Wallets::onGetRawPrivateKey);
    Q_CONNECT(this, &Wallets::createRsaKey, this, &Wallets::onCreateRsaKey);
    Q_CONNECT(this, &Wallets::getRsaPublicKey, this, &Wallets::onGetRsaPublicKey);
    Q_CONNECT(this, &Wallets::copyRsaKey, this, &Wallets::onCopyRsaKey);
    Q_CONNECT(this, &Wallets::copyRsaKeyToFolder, this, &Wallets::onCopyRsaKeyToFolder);
    Q_CONNECT(this, &Wallets::createEthKey, this, &Wallets::onCreateEthKey);
    Q_CONNECT(this, &Wallets::signMessageEth, this, &Wallets::onSignMessageEth);
    Q_CONNECT(this, &Wallets::checkAddressEth, this, &Wallets::onCheckAddressEth);
    Q_CONNECT(this, &Wallets::savePrivateKeyEth, this, &Wallets::onSavePrivateKeyEth);
    Q_CONNECT(this, &Wallets::getOnePrivateKeyEth, this, &Wallets::onGetOnePrivateKeyEth);
    Q_CONNECT(this, &Wallets::createBtcKey, this, &Wallets::onCreateBtcKey);
    Q_CONNECT(this, &Wallets::checkAddressBtc, this, &Wallets::onCheckAddressBtc);
    Q_CONNECT(this, &Wallets::signMessageBtcUsedUtxos, this, &Wallets::onSignMessageBtcUsedUtxos);
    Q_CONNECT(this, &Wallets::savePrivateKeyBtc, this, &Wallets::onSavePrivateKeyBtc);
    Q_CONNECT(this, &Wallets::getOnePrivateKeyBtc, this, &Wallets::onGetOnePrivateKeyBtc);
    Q_CONNECT(this, &Wallets::getWalletFolders, this, &Wallets::onGetWalletFolders);
    Q_CONNECT(this, &Wallets::backupKeys, this, &Wallets::onBackupKeys);
    Q_CONNECT(this, &Wallets::restoreKeys, this, &Wallets::onRestoreKeys);
    Q_CONNECT(this, &Wallets::openWalletPathInStandartExplorer, this, &Wallets::onOpenWalletPathInStandartExplorer);
    Q_CONNECT(this, &Wallets::importKeys, this, &Wallets::onImportKeys);
    Q_CONNECT(this, &Wallets::calkKeys, this, &Wallets::onCalkKeys);
    Q_CONNECT(this, &Wallets::importKeysEth, this, &Wallets::onImportKeysEth);
    Q_CONNECT(this, &Wallets::calkKeysEth, this, &Wallets::onCalkKeysEth);
    Q_CONNECT(this, &Wallets::importKeysBtc, this, &Wallets::onImportKeysBtc);
    Q_CONNECT(this, &Wallets::calkKeysBtc, this, &Wallets::onCalkKeysBtc);

    Q_REG(WalletsListCallback, "WalletsListCallback");
    Q_REG(wallets::WalletCurrency, "wallets::WalletCurrency");
    Q_REG2(std::vector<QString>, "std::vector<QString>", false);
    Q_REG(CreateWatchsCallback, "CreateWatchsCallback");
    Q_REG(CreateWalletCallback, "CreateWalletCallback");
    Q_REG(CreateWatchWalletCallback, "CreateWatchWalletCallback");
    Q_REG(RemoveWatchWalletCallback, "RemoveWatchWalletCallback");
    Q_REG(CheckWalletExistCallback, "CheckWalletExistCallback");
    Q_REG(CheckWalletPasswordCallback, "CheckWalletPasswordCallback");
    Q_REG(CheckAddressCallback, "CheckAddressCallback");
    Q_REG(CreateContractAddressCallback, "CreateContractAddressCallback");
    Q_REG(wallets::Wallets::SignMessageCallback, "wallets::Wallets::SignMessageCallback");
    Q_REG(SignMessage2Callback, "SignMessage2Callback");
    Q_REG(GettedNonceCallback, "GettedNonceCallback");
    Q_REG(SignAndSendMessageCallback, "SignAndSendMessageCallback");
    Q_REG(GetPrivateKeyCallback, "GetPrivateKeyCallback");
    Q_REG(SavePrivateKeyCallback, "SavePrivateKeyCallback");
    Q_REG(SaveRawPrivateKeyCallback, "SaveRawPrivateKeyCallback");
    Q_REG(GetRawPrivateKeyCallback, "GetRawPrivateKeyCallback");
    Q_REG(CreateRsaKeyCallback, "CreateRsaKeyCallback");
    Q_REG(GetRsaPublicKeyCallback, "GetRsaPublicKeyCallback");
    Q_REG(CopyRsaKeyCallback, "CopyRsaKeyCallback");
    Q_REG(CreateEthKeyCallback, "CreateEthKeyCallback");
    Q_REG(SignMessageEthCallback, "SignMessageEthCallback");
    Q_REG(CreateBtcKeyCallback, "CreateBtcKeyCallback");
    Q_REG(SignMessageBtcCallback, "SignMessageBtcCallback");
    Q_REG2(std::set<std::string>, "std::set<std::string>", false);
    Q_REG2(std::vector<BtcInput>, "std::vector<BtcInput>", false);
    Q_REG(GetWalletFoldersCallback, "GetWalletFoldersCallback");
    Q_REG(BackupKeysCallback, "BackupKeysCallback");
    Q_REG(RestoreKeysCallback, "RestoreKeysCallback");
    Q_REG(ImportKeysCallback, "ImportKeysCallback");
    Q_REG(CalkKeysCallback, "CalkKeysCallback");
    Q_REG2(WalletsList, "WalletsList", false);

    emit auth.reEmit();

    fileSystemWatcher.moveToThread(TimerClass::getThread());
    moveToThread(TimerClass::getThread()); // TODO вызывать в TimerClass
}

Wallets::~Wallets() {
    TimerClass::exit();
}

void Wallets::setTransactions(transactions::Transactions *txs) {
    this->txs = txs;
}

///////////
/// MTH ///
///////////

void Wallets::onCreateWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        WalletsList created;
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        for (const QString &addr : addresses) {
            if (Wallet::isWalletExists(walletPath, isMhc, addr.toStdString())) {
                continue;
            }
            Wallet::createWalletWatch(walletPath, isMhc, addr.toStdString());
            Wallet wallet(walletPath, isMhc, addr.toStdString());
            const QString walletFullPath = wallet.getFullPath();
            created.emplace_back(addr, walletFullPath);

            walletsList[isMhc ? WalletCurrency::Mth : WalletCurrency::Tmh][addr] = WalletInfo(addr, walletFullPath, WalletInfo::Type::Watch);
        }

        if (!created.empty()) {
            emit watchWalletsAdded(isMhc, created, userName);
        }

        return created;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCreateWallet(bool isMhc, const QString &password, const CreateWalletCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        const std::string exampleMessage = "Example message " + std::to_string(rand());

        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        std::string pKey;
        std::string addr;
        Wallet::createWallet(walletPath, isMhc, password.normalized(QString::NormalizationForm_C).toStdString(), pKey, addr);

        pKey.clear();
        Wallet wallet(walletPath, isMhc, addr, password.normalized(QString::NormalizationForm_C).toStdString());
        const std::string signature = wallet.sign(exampleMessage, pKey);

        const QString walletFullPath = wallet.getFullPath();

        walletsList[isMhc ? WalletCurrency::Mth : WalletCurrency::Tmh][QString::fromStdString(addr)] = WalletInfo(QString::fromStdString(addr), walletFullPath, WalletInfo::Type::Key);

        emit mhcWalletCreated(isMhc, QString::fromStdString(addr), userName);

        return std::make_tuple(walletFullPath, pKey, addr, exampleMessage, signature);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCreateWatchWallet(bool isMhc, const QString &address, const CreateWatchWalletCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createWalletWatch(walletPath, isMhc, address.toStdString());

        Wallet wallet(walletPath, isMhc, address.toStdString());

        const QString walletFullPath = wallet.getFullPath();

        walletsList[isMhc ? WalletCurrency::Mth : WalletCurrency::Tmh][address] = WalletInfo(address, walletFullPath, WalletInfo::Type::Watch);

        emit mhcWatchWalletCreated(isMhc, address, userName);

        return walletFullPath;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onRemoveWatchWallet(bool isMhc, const QString &address, const RemoveWatchWalletCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::removeWalletWatch(walletPath, isMhc, address.toStdString());

        walletsList[isMhc ? WalletCurrency::Mth : WalletCurrency::Tmh].erase(address);

        emit mhcWatchWalletRemoved(isMhc, address, userName);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCheckWalletExist(bool isMhc, const QString &address, const CheckWalletExistCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        return Wallet::isWalletExists(walletPath, isMhc, address.toStdString());
    }, callback, false);
END_SLOT_WRAPPER
}

void Wallets::onCheckWalletPassword(bool isMhc, const QString &address, const QString &password, const CheckWalletPasswordCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        const std::string exampleMessage = "Example message " + std::to_string(rand());

        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");

        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString());
        std::string tmp;
        const std::string signature = wallet.sign(exampleMessage, tmp);

        return true;
    }, callback, false);
END_SLOT_WRAPPER
}

void Wallets::onCheckAddress(const QString &address, const CheckAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        try {
            Wallet::checkAddress(address.toStdString());
            return true;
        } catch (const Exception &e) {
            return false;
        } catch (...) {
            throw;
        }
    }, callback, false);
END_SLOT_WRAPPER
}

void Wallets::onCreateContractAddress(const QString &address, int nonce, const CreateContractAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return QString::fromStdString(Wallet::createV8Address(address.toStdString(), nonce));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSignMessage(bool isMhc, const QString &address, const QString &text, const QString &password, const SignMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString());
        std::string pubKey;
        const QString signature = QString::fromStdString(wallet.sign(text.toStdString(), pubKey));
        const QString publicKey = QString::fromStdString(pubKey);
        return std::make_tuple(signature, publicKey);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSignMessage2(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const SignMessage2Callback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");

        QString realFee = fee;
        if (realFee.isEmpty()) {
            realFee = "0";
        }

        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString());
        std::string publicKey;
        std::string tx;
        std::string signature;
        bool tmp;
        const uint64_t valueInt = value.toULongLong(&tmp, 10);
        CHECK(tmp, "Value not valid");
        const uint64_t feeInt = realFee.toULongLong(&tmp, 10);
        CHECK(tmp, "Fee not valid");
        const uint64_t nonceInt = nonce.toULongLong(&tmp, 10);
        CHECK(tmp, "Nonce not valid");
        wallet.sign(toAddress.toStdString(), valueInt, feeInt, nonceInt, dataHex.toStdString(), tx, signature, publicKey);
        const QString publicKey2 = QString::fromStdString(publicKey);
        const QString tx2 = QString::fromStdString(tx);
        const QString signature2 = QString::fromStdString(signature);

        return std::make_tuple(signature2, publicKey2, tx2);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::findNonceAndProcessWithTxManager(const QString &address, const QString &nonce, const transactions::SendParameters &sendParams, const GettedNonceCallback &callback) {
    const bool isNonce = !nonce.isEmpty();
    if (!isNonce) {
        CHECK(txs != nullptr, "Transactions manager not setted");
        emit txs->getNonce(address, sendParams, transactions::Transactions::GetNonceCallback([callback, address](size_t nonce, const QString &serverError) {
            LOG << "Nonce getted " << address << " " << nonce << " " << serverError;
            callback.emitCallback(nonce);
        }, callback, signalFunc));
    } else {
        bool isParseNonce = false;
        const size_t nonceInt = nonce.toULongLong(&isParseNonce);
        CHECK_TYPED(isParseNonce, TypeErrors::INCORRECT_USER_DATA, "Nonce incorrect " + nonce.toStdString());
        callback.emitCallback(nonceInt);
    }
}

void Wallets::onSignAndSendMessage(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &paramsJson, const SignAndSendMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const transactions::SendParameters sendParams = transactions::parseSendParams(paramsJson);

        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString()); // Проверяем пароль кошелька

        QString realFee = fee;
        if (realFee.isEmpty()) {
            realFee = "0";
        }

        const QString walletPathCopy = walletPath;

        const auto signTransaction = [this, walletPathCopy, isMhc, address, password, toAddress, value, realFee, dataHex, sendParams, callback](size_t nonce) {
            Wallet wallet(walletPathCopy, isMhc, address.toStdString(), password.toStdString());
            std::string publicKey;
            std::string tx;
            std::string signature;
            bool tmp;
            const uint64_t valueInt = value.toULongLong(&tmp, 10);
            CHECK(tmp, "Value not valid");
            const uint64_t feeInt = realFee.toULongLong(&tmp, 10);
            CHECK(tmp, "Fee not valid");
            wallet.sign(toAddress.toStdString(), valueInt, feeInt, nonce, dataHex.toStdString(), tx, signature, publicKey);

            CHECK(txs != nullptr, "Transactions manager not setted");
            emit txs->sendTransaction("", toAddress, value, nonce, dataHex, realFee, QString::fromStdString(publicKey), QString::fromStdString(signature), sendParams, transactions::Transactions::SendTransactionCallback([address, callback](){
                callback.emitCallback(true);
            }, callback, signalFunc));
        };

        findNonceAndProcessWithTxManager(address, nonce, sendParams, GettedNonceCallback(signTransaction, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSignAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson, const SignAndSendMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const transactions::SendParameters sendParams = transactions::parseSendParams(paramsJson);

        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString()); // Проверяем пароль кошелька

        QString realFee = fee;
        if (realFee.isEmpty()) {
            realFee = "0";
        }

        const QString walletPathCopy = walletPath;

        const auto signTransaction = [this, walletPathCopy, isMhc, address, password, toAddress, value, realFee, valueDelegate, isDelegate, sendParams, callback](size_t nonce) {
            Wallet wallet(walletPathCopy, isMhc, address.toStdString(), password.toStdString());

            bool isValid;
            const uint64_t delegValue = valueDelegate.toULongLong(&isValid);
            CHECK(isValid, "delegate value not valid");
            const std::string dataHex = Wallet::genDataDelegateHex(isDelegate, delegValue);

            std::string publicKey;
            std::string tx;
            std::string signature;
            bool tmp;
            const uint64_t valueInt = value.toULongLong(&tmp, 10);
            CHECK(tmp, "Value not valid");
            const uint64_t feeInt = realFee.toULongLong(&tmp, 10);
            CHECK(tmp, "Fee not valid");
            wallet.sign(toAddress.toStdString(), valueInt, feeInt, nonce, dataHex, tx, signature, publicKey, false);

            CHECK(txs != nullptr, "Transactions manager not setted");
            emit txs->sendTransaction("", toAddress, value, nonce, QString::fromStdString(dataHex), realFee, QString::fromStdString(publicKey), QString::fromStdString(signature), sendParams, transactions::Transactions::SendTransactionCallback([address, callback](){
                callback.emitCallback(true);
            }, callback, signalFunc));
        };

        findNonceAndProcessWithTxManager(address, nonce, sendParams, GettedNonceCallback(signTransaction, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetOnePrivateKey(bool isMhc, const QString &address, bool isCompact, const GetPrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");

        return QString::fromStdString(Wallet::getPrivateKey(walletPath, isMhc, address.toStdString(), isCompact));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSavePrivateKey(bool isMhc, const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");

        std::string key = privateKey.toStdString();
        if (key.compare(0, Wallet::PREFIX_ONE_KEY_MTH.size(), Wallet::PREFIX_ONE_KEY_MTH) == 0) {
            key = key.substr(Wallet::PREFIX_ONE_KEY_MTH.size());
        } else if (key.compare(0, Wallet::PREFIX_ONE_KEY_TMH.size(), Wallet::PREFIX_ONE_KEY_TMH) == 0) {
            key = key.substr(Wallet::PREFIX_ONE_KEY_TMH.size());
        }

        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const std::string address = Wallet::savePrivateKey(walletPath, isMhc, key, password.toStdString());

        return std::make_tuple(true, QString::fromStdString(address));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSaveRawPrivateKey(bool isMhc, const QString &rawPrivateKey, const QString &password, const SaveRawPrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        std::string addr;
        std::string pubkey;
        Wallet::createWalletFromRaw(walletPath, isMhc, rawPrivateKey.toStdString(), password.normalized(QString::NormalizationForm_C).toStdString(), pubkey, addr);

        return QString::fromStdString(addr);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetRawPrivateKey(bool isMhc, const QString &address, const QString &password, const GetRawPrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString());
        return QString::fromStdString(wallet.getNotProtectedKeyHex());
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onImportKeys(bool isMhc, const QString &path, const ImportKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const int result = importKeysImpl(path, [](const QString &filePath) {
            return Wallet::isCorrectFilenameWallet(filePath);
        }, [this, isMhc](const QString &fileName) {
            copyToDirectoryFile(fileName, makePath(walletPath, Wallet::chooseSubfolder(isMhc)), true);
        });
        return result;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCalkKeys(bool /*isMhc*/, const QString &path, const CalkKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        std::vector<QString> result;
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        importKeysImpl(path, [](const QString &filePath) {
            return Wallet::isCorrectFilenameWallet(filePath);
        }, [&result](const QString &filePath) {
            const QString fileName = getFileName(filePath);
            result.emplace_back(fileName);
        });
        return result;
    }, callback);
END_SLOT_WRAPPER
}

///////////
/// RSA ///
///////////

void Wallets::onCreateRsaKey(bool isMhc, const QString &address, const QString &password, const CreateRsaKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        WalletRsa::createRsaKey(walletPath, isMhc, address.toStdString(), password.toStdString());
        WalletRsa wallet(walletPath, isMhc, address.toStdString());
        return QString::fromStdString(wallet.getPublikKey());
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetRsaPublicKey(bool isMhc, const QString &address, const GetRsaPublicKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        WalletRsa wallet(walletPath, isMhc, address.toStdString());
        return QString::fromStdString(wallet.getPublikKey());
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCopyRsaKey(bool isMhc, const QString &address, const QString &pathPub, const QString &pathPriv, const CopyRsaKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        CHECK(WalletRsa::validateKeyName(pathPriv, pathPub, address), "Not rsa key");
        const QString newFolder = WalletRsa::genFolderRsa(walletPath, isMhc);
        copyToDirectoryFile(pathPub, newFolder, false);
        copyToDirectoryFile(pathPriv, newFolder, false);
        return true;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCopyRsaKeyToFolder(bool isMhc, const QString &address, const QString &path, const CopyRsaKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<QString> files = WalletRsa::getPathsKeys(walletPath, isMhc, address);
        for (const QString &file: files) {
            CHECK(isExistFile(file), "Key not found");
            copyToDirectoryFile(file, path, false);
        }
        return true;
    }, callback);
END_SLOT_WRAPPER
}

////////////////
/// ETHEREUM ///
////////////////

void Wallets::onCreateEthKey(const QString &password, const CreateEthKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const QString address = QString::fromStdString(EthWallet::genPrivateKey(walletPath, password.normalized(QString::NormalizationForm_C).toStdString()));

        const QString fullPath = EthWallet::getFullPath(walletPath, address.toStdString());

        walletsList[WalletCurrency::Eth][address] = WalletInfo(address, fullPath, WalletInfo::Type::Key);

        return std::make_tuple(address, fullPath);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSignMessageEth(const QString &address, const QString &password, const QString &nonce, const QString &gasPrice, const QString &gasLimit, const QString &to, const QString &value, const QString &data, const SignMessageEthCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        EthWallet wallet(walletPath, address.toStdString(), password.toStdString());
        return QString::fromStdString(wallet.SignTransaction(
            nonce.toStdString(),
            gasPrice.toStdString(),
            gasLimit.toStdString(),
            to.toStdString(),
            value.toStdString(),
            data.toStdString()
        ));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCheckAddressEth(const QString &address, const CheckAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        try {
            EthWallet::checkAddress(address.toStdString());
            return true;
        } catch (const Exception &) {
            return false;
        } catch (...) {
            throw;
        }
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSavePrivateKeyEth(const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        std::string key = privateKey.toStdString();
        if (key.compare(0, EthWallet::PREFIX_ONE_KEY.size(), EthWallet::PREFIX_ONE_KEY) == 0) {
            key = key.substr(EthWallet::PREFIX_ONE_KEY.size());
        }

        const std::string address = EthWallet::savePrivateKey(walletPath, privateKey.toStdString(), password.toStdString());
        return std::make_tuple(true, QString::fromStdString(address));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetOnePrivateKeyEth(const QString &address, const GetPrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        return QString::fromStdString(EthWallet::getOneKey(walletPath, address.toStdString()));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onImportKeysEth(const QString &path, const ImportKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const int result = importKeysImpl(path, [](const QString &filePath) {
            return EthWallet::isCorrectFilenameWallet(filePath);
        }, [this](const QString &fileName) {
            copyToDirectoryFile(fileName, makePath(walletPath, EthWallet::subfolder()), true);
        });
        return result;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCalkKeysEth(const QString &path, const CalkKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        std::vector<QString> result;
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        importKeysImpl(path, [](const QString &filePath) {
            return EthWallet::isCorrectFilenameWallet(filePath);
        }, [&result](const QString &filePath) {
            const QString fileName = getFileName(filePath);
            result.emplace_back(fileName);
        });
        return result;
    }, callback);
END_SLOT_WRAPPER
}

///////////
/// BTC ///
///////////

void Wallets::onCreateBtcKey(const QString &password, const CreateBtcKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = BtcWallet::genPrivateKey(walletPath, password).first;

        const QString fullPath = BtcWallet::getFullPath(walletPath, address);

        walletsList[WalletCurrency::Btc][QString::fromStdString(address)] = WalletInfo(QString::fromStdString(address), fullPath, WalletInfo::Type::Key);

        return std::make_tuple(QString::fromStdString(address), fullPath);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCheckAddressBtc(const QString &address, const CheckAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        try {
            BtcWallet::checkAddress(address.toStdString());
            return true;
        } catch (const Exception &) {
            return false;
        } catch (...) {
            throw;
        }
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSignMessageBtcUsedUtxos(const QString &address, const QString &password, const std::vector<BtcInput> &inputs, const QString &toAddress, const QString &value, const QString &estimateComissionInSatoshi, const QString &fees, const std::set<std::string> &usedUtxos, const SignMessageBtcCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        std::vector<BtcInput> btcInputs = BtcWallet::reduceInputs(inputs, usedUtxos);

        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        BtcWallet wallet(walletPath, address.toStdString(), password);
        size_t estimateComissionInSatoshiInt = 0;
        if (!estimateComissionInSatoshi.isEmpty()) {
            CHECK(isDecimal(estimateComissionInSatoshi.toStdString()), "Not hex number value");
            estimateComissionInSatoshiInt = std::stoull(estimateComissionInSatoshi.toStdString());
        }
        const auto resultPair = wallet.buildTransaction(btcInputs, estimateComissionInSatoshiInt, value.toStdString(), fees.toStdString(), toAddress.toStdString());
        const std::string result = resultPair.first;
        const std::set<std::string> &thisUsedTxs = resultPair.second;

        std::set<std::string> newUtxos = usedUtxos;
        newUtxos.insert(thisUsedTxs.begin(), thisUsedTxs.end());

        const std::string transactionHash = BtcWallet::calcHashNotWitness(result);

        return std::make_tuple(QString::fromStdString(result), QString::fromStdString(transactionHash), newUtxos);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSavePrivateKeyBtc(const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        std::string key = privateKey.toStdString();
        if (key.compare(0, BtcWallet::PREFIX_ONE_KEY.size(), BtcWallet::PREFIX_ONE_KEY) == 0) {
            key = key.substr(BtcWallet::PREFIX_ONE_KEY.size());
        }

        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const std::string address = BtcWallet::savePrivateKey(walletPath, privateKey.toStdString(), password);

        return std::make_tuple(true, QString::fromStdString(address));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetOnePrivateKeyBtc(const QString &address, const GetPrivateKeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::string privKey = BtcWallet::getOneKey(walletPath, address.toStdString());

        return QString::fromStdString(privKey);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onImportKeysBtc(const QString &path, const ImportKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const int result = importKeysImpl(path, [](const QString &filePath) {
            return BtcWallet::isCorrectFilenameWallet(filePath);
        }, [this](const QString &fileName) {
            copyToDirectoryFile(fileName, makePath(walletPath, BtcWallet::subfolder()), true);
        });
        return result;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCalkKeysBtc(const QString &path, const CalkKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        std::vector<QString> result;
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        importKeysImpl(path, [](const QString &filePath) {
            return BtcWallet::isCorrectFilenameWallet(filePath);
        }, [&result](const QString &filePath) {
            const QString fileName = QString::fromStdString(BtcWallet::getAddress(filePath));
            result.emplace_back(fileName);
        });
        return result;
    }, callback);
END_SLOT_WRAPPER
}

//////////////
/// COMMON ///
//////////////

void Wallets::onGetListWallets(const WalletCurrency &type, const WalletsListCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Wallet path not set");
        const auto found = walletsList.find(type);
        if (found == walletsList.end()) {
            throwErr("Incorrect type or not found wallets");
        }
        std::vector<WalletInfo> result;
        result.reserve(found->second.size());
        std::transform(found->second.begin(), found->second.end(), std::back_inserter(result), [](const auto &pair) {
            return pair.second;
        });
        return std::make_tuple(userName, result);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetListWallets2(const wallets::WalletCurrency &type, const QString &expectedUsername, const WalletsListCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        if (expectedUsername == userName) {
            emit getListWallets(type, callback);
        } else {
            std::unique_ptr<GetActualWalletsEvent> event = std::make_unique<GetActualWalletsEvent>(TimerClass::getThread(), *this, expectedUsername, type, callback);

            Q_CONNECT(this, &Wallets::usernameChanged, event.get(), &GetActualWalletsEvent::changedUserName);

            eventWatcher.addEvent("getListWallet", std::move(event), 3s);
        }
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onGetWalletFolders(const GetWalletFoldersCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return std::make_tuple(walletDefaultPath, walletPath, userName);
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onBackupKeys(const QString &caption, const BackupKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const QString beginPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "backup.zip");
        const QString walletPathCopy = walletPath;
        emit utils.saveFileDialog(caption, beginPath, utils::Utils::ChooseFileCallback([walletPathCopy, callback](const QString &fileName) {
            if (!fileName.isEmpty()) {
                ::backupKeys(walletPathCopy, fileName);
                callback.emitCallback(fileName);
            } else {
                callback.emitCallback("");
            }
        }, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onRestoreKeys(const QString &caption, const RestoreKeysCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const QString walletPathCopy = walletPath;
        const QString beginPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "backup.zip");
        emit utils.loadFileDialog(caption, beginPath, "*.zip;;*.*", utils::Utils::ChooseFileCallback([this, walletPathCopy, caption, callback](const QString &fileName) {
            if (fileName.isEmpty()) {
                callback.emitCallback("");
                return;
            }
            const std::string text = checkBackupFile(fileName);
            emit utils.question(caption, "Restore backup " + QString::fromStdString(text) + "?", utils::Utils::QuestionCallback([walletPathCopy, fileName, callback](bool result) {
                if (result) {
                    ::restoreKeys(fileName, walletPathCopy);
                    callback.emitCallback(fileName);
                } else {
                    callback.emitCallback("");
                }
            }, callback, signalFunc));
        }, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onOpenWalletPathInStandartExplorer() {
BEGIN_SLOT_WRAPPER
    CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
    emit utils.openFolderInStandartExplored(walletPath);
END_SLOT_WRAPPER
}

///////////////
/// METHODS ///
///////////////

void Wallets::startMethod() {

}

void Wallets::timerMethod() {
    eventWatcher.checkEvents();
}

void Wallets::finishMethod() {

}

std::vector<WalletInfo> Wallets::readAllWallets(const WalletCurrency &type) {
    CHECK(!walletPath.isEmpty(), "Wallet path not set");
    if (type == WalletCurrency::Tmh) {
        return Wallet::getAllWalletsInfoInFolder(walletPath, false);
    } else if (type == WalletCurrency::Mth) {
        return Wallet::getAllWalletsInfoInFolder(walletPath, true);
    } else if (type == WalletCurrency::Btc) {
        const WalletsList res = BtcWallet::getAllWalletsInFolder(walletPath);
        std::vector<WalletInfo> result;
        result.reserve(res.size());
        std::transform(res.begin(), res.end(), std::back_inserter(result), [](const auto &pair) {
            return WalletInfo(pair.first, pair.second, WalletInfo::Type::Key);
        });
        return result;
    } else if (type == WalletCurrency::Eth) {
        const WalletsList res = EthWallet::getAllWalletsInFolder(walletPath);
        std::vector<WalletInfo> result;
        result.reserve(res.size());
        std::transform(res.begin(), res.end(), std::back_inserter(result), [](const auto &pair) {
            return WalletInfo(pair.first, pair.second, WalletInfo::Type::Key);
        });
        return result;
    } else {
        throwErr("Incorrect type");
    }
}

int Wallets::importKeysImpl(const QString &path, const std::function<bool(const QString &filePath)> &checkFileName, const std::function<void(const QString &path)> &processFile) {
    if (isDirectory(path)) {
        const QStringList files = getFilesForDir(path);
        int countSuccess = 0;
        for (const QString &fileName: files) {
            const QString filePath = makePath(path, fileName);
            const bool isKey = checkFileName(filePath);
            if (isKey) {
                processFile(filePath);
                countSuccess++;
            }
        }
        return countSuccess;
    } else {
        const bool isKey = checkFileName(path);
        if (!isKey) {
            return 0;
        } else {
            processFile(path);
            return 1;
        }
    }
}

static std::map<QString, WalletInfo> walletsToMap(const std::vector<WalletInfo> &wallets) {
    std::map<QString, WalletInfo> wallets2;
    std::transform(wallets.begin(), wallets.end(), std::inserter(wallets2, wallets2.begin()), [](const WalletInfo &info) {
        return std::make_pair(info.address, info);
    });
    return wallets2;
}

void Wallets::setPathsImpl(QString newPatch, QString newUserName) {
    userName = newUserName;

    if (walletPath == newPatch) {
        return;
    }

    walletPath = newPatch;
    CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
    createFolder(walletPath);

    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        fileSystemWatcher.removePath(folderInfo.walletPath.absolutePath());
    }
    folderWalletsInfos.clear();

    const auto setPathToWallet = [this](const QString &suffix, const QString &name, const WalletCurrency &type) {
        const QString curPath = makePath(walletPath, suffix);
        createFolder(curPath);
        folderWalletsInfos.emplace_back(curPath, name);
        fileSystemWatcher.addPath(curPath);

        const std::vector<WalletInfo> wallets = readAllWallets(type);
        walletsList[type] = walletsToMap(wallets);
    };

    walletsList.clear();

    setPathToWallet(EthWallet::subfolder(), "eth", WalletCurrency::Eth);
    setPathToWallet(BtcWallet::subfolder(), "btc", WalletCurrency::Btc);
    setPathToWallet(Wallet::chooseSubfolder(true), "mhc", WalletCurrency::Mth);
    setPathToWallet(Wallet::chooseSubfolder(false), "tmh", WalletCurrency::Tmh);

    LOG << "Wallets path " << walletPath;

    emit usernameChanged(userName);
}

void Wallets::onLogined(bool /*isInit*/, const QString &login, const QString &token_) {
BEGIN_SLOT_WRAPPER
    if (!login.isEmpty()) {
        setPathsImpl(makePath(walletDefaultPath, login), login);
        token = token_;
    } else {
        setPathsImpl(makePath(walletDefaultPath, "_unregistered"), "");
    }
END_SLOT_WRAPPER
}

void Wallets::onDirChanged(const QString &dir) {
BEGIN_SLOT_WRAPPER
    const QDir changedPath = dir;
    WalletCurrency currency;
    if (changedPath == QDir(makePath(walletPath, EthWallet::subfolder()))) {
        currency = WalletCurrency::Eth;
    } else if (changedPath == QDir(makePath(walletPath, BtcWallet::subfolder()))) {
        currency = WalletCurrency::Btc;
    } else if (changedPath == QDir(makePath(walletPath, Wallet::chooseSubfolder(true)))) {
        currency = WalletCurrency::Mth;
    } else if (changedPath == QDir(makePath(walletPath, Wallet::chooseSubfolder(false)))) {
        currency = WalletCurrency::Tmh;
    } else {
        return;
    }

    const std::vector<WalletInfo> currentWallets = readAllWallets(currency);
    const std::map<QString, WalletInfo> currentWallets2 = walletsToMap(currentWallets);

    const auto &walletsL = walletsList[currency];
    for (const auto &pair: currentWallets2) {
        if (walletsL.find(pair.first) == walletsL.end()) {
            if (currency == WalletCurrency::Mth || currency == WalletCurrency::Tmh) {
                const bool isMhc = currency == WalletCurrency::Mth;
                if (pair.second.type == WalletInfo::Type::Watch) {
                    emit mhcWatchWalletCreated(isMhc, pair.first, userName);
                } else {
                    emit mhcWalletCreated(isMhc, pair.first, userName);
                }
            }
        }
    }

    for (const auto &pair: walletsL) {
        if (currentWallets2.find(pair.first) == currentWallets2.end()) {
            if (currency == WalletCurrency::Mth || currency == WalletCurrency::Tmh) {
                const bool isMhc = currency == WalletCurrency::Mth;
                if (pair.second.type == WalletInfo::Type::Watch) {
                    emit mhcWatchWalletRemoved(isMhc, pair.first, userName);
                }
            }
        }
    }

    walletsList[currency] = currentWallets2;

    const QDir d(dir);
    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        if (folderInfo.walletPath == d) {
            emit dirChanged(d.absolutePath(), folderInfo.nameWallet);
        }
    }
END_SLOT_WRAPPER
}

} // namespace wallets
