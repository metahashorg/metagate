#include "Wallets.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "Log.h"
#include "check.h"

#include "utilites/utils.h"

#include "Wallet.h"
#include "BtcWallet.h"
#include "EthWallet.h"
#include "WalletRsa.h"

#include "Paths.h"

#include "auth/Auth.h"

#include "transactions/Transactions.h"

#include "GetActualWalletsEvent.h"

SET_LOG_NAMESPACE("WLTS");

namespace wallets {

const QString Wallets::defaultUsername = "_unregistered";

Wallets::Wallets(auth::Auth &auth, QObject *parent)
    : TimerClass(5s, parent)
    , walletDefaultPath(getWalletPath())
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
    Q_REG(SignMessageCallback, "SignMessageCallback");
    Q_REG(SignMessage2Callback, "SignMessage2Callback");
    Q_REG(GettedNonceCallback, "GettedNonceCallback");
    Q_REG(SignAndSendMessageCallback, "SignAndSendMessageCallback");

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

void Wallets::onGetListWallets(const WalletCurrency &type, const WalletsListCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Wallet path not set");
        if (type == WalletCurrency::Tmh) {
            return std::make_tuple(userName, Wallet::getAllWalletsInfoInFolder(walletPath, false));
        } else if (type == WalletCurrency::Mth) {
            return std::make_tuple(userName, Wallet::getAllWalletsInfoInFolder(walletPath, true));
        } else if (type == WalletCurrency::Btc) {
            const std::vector<std::pair<QString, QString>> res = BtcWallet::getAllWalletsInFolder(walletPath);
            std::vector<WalletInfo> result;
            result.reserve(res.size());
            std::transform(res.begin(), res.end(), std::back_inserter(result), [](const auto &pair) {
                return WalletInfo(pair.first, pair.second, WalletInfo::Type::Key);
            });
            return std::make_tuple(userName, result);
        } else if (type == WalletCurrency::Eth) {
            const std::vector<std::pair<QString, QString>> res = EthWallet::getAllWalletsInFolder(walletPath);
            std::vector<WalletInfo> result;
            result.reserve(res.size());
            std::transform(res.begin(), res.end(), std::back_inserter(result), [](const auto &pair) {
                return WalletInfo(pair.first, pair.second, WalletInfo::Type::Key);
            });
            return std::make_tuple(userName, result);
        } else {
            throwErr("Incorrect type");
        }
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

void Wallets::onCreateWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        std::vector<std::pair<QString, QString>> created;
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        for (const QString &addr : addresses) {
            if (Wallet::isWalletExists(walletPath, isMhc, addr.toStdString())) {
                continue;
            }
            Wallet::createWalletWatch(walletPath, isMhc, addr.toStdString());
            Wallet wallet(walletPath, isMhc, addr.toStdString());
            const QString walletFullPath = wallet.getFullPath();
            created.emplace_back(std::make_pair(addr, walletFullPath));
        }

        emit watchWalletsAdded(isMhc, created);

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

        emit mhcWalletCreated(isMhc, QString::fromStdString(addr));

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

        emit mhcWatchWalletCreated(isMhc, address);

        return walletFullPath;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onRemoveWatchWallet(bool isMhc, const QString &address, const RemoveWatchWalletCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::removeWalletWatch(walletPath, isMhc, address.toStdString());

        emit mhcWatchWalletRemoved(isMhc, address);
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

void Wallets::findNonceAndProcessWithTxManager(bool isMhc, const QString &address, const QString &nonce, const QString &paramsJson, const GettedNonceCallback &callback) {
    const transactions::SendParameters sendParams = transactions::parseSendParams(paramsJson);

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
        const transactions::SendParameters sendParams = transactions::parseSendParams(paramsJson);

        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString()); // Проверяем пароль кошелька

        QString realFee = fee;
        if (realFee.isEmpty()) {
            realFee = "0";
        }

        const auto signTransaction = [this, walletPath=this->walletPath, isMhc, address, password, toAddress, value, realFee, dataHex, sendParams, callback](size_t nonce) {
            Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString());
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

        findNonceAndProcessWithTxManager(isMhc, address, nonce, paramsJson, GettedNonceCallback(signTransaction, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onSignAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson, const SignAndSendMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        const transactions::SendParameters sendParams = transactions::parseSendParams(paramsJson);

        Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString()); // Проверяем пароль кошелька

        QString realFee = fee;
        if (realFee.isEmpty()) {
            realFee = "0";
        }

        const auto signTransaction = [this, walletPath=this->walletPath, isMhc, address, password, toAddress, value, realFee, valueDelegate, isDelegate, sendParams, callback](size_t nonce) {
            Wallet wallet(walletPath, isMhc, address.toStdString(), password.toStdString());

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

        findNonceAndProcessWithTxManager(isMhc, address, nonce, paramsJson, GettedNonceCallback(signTransaction, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::startMethod() {

}

void Wallets::timerMethod() {
    eventWatcher.checkEvents();
}

void Wallets::finishMethod() {

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

    const auto setPathToWallet = [this](const QString &suffix, const QString &name) {
        const QString curPath = makePath(walletPath, suffix);
        createFolder(curPath);
        folderWalletsInfos.emplace_back(curPath, name);
        fileSystemWatcher.addPath(curPath);
    };

    setPathToWallet(EthWallet::subfolder(), "eth");
    setPathToWallet(BtcWallet::subfolder(), "btc");
    setPathToWallet(Wallet::chooseSubfolder(true), "mhc");
    setPathToWallet(Wallet::chooseSubfolder(false), "tmh");

    LOG << "Wallets path " << walletPath;

    emit usernameChanged(userName);
}

void Wallets::onLogined(bool /*isInit*/, const QString &login, const QString &token_) {
BEGIN_SLOT_WRAPPER
    if (!login.isEmpty()) {
        setPathsImpl(makePath(walletDefaultPath, login), login);
        token = token_;
    } else {
        setPathsImpl(makePath(walletDefaultPath, defaultUsername), defaultUsername);
    }
END_SLOT_WRAPPER
}

void Wallets::onDirChanged(const QString &dir) {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
}

} // namespace wallets
