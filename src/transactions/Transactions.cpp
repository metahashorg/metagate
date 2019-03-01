#include "Transactions.h"

#include <functional>
using namespace std::placeholders;

#include <QSettings>

#include "check.h"
#include "SlotWrapper.h"
#include "Paths.h"
#include "QRegister.h"

#include "NsLookup.h"

#include "TransactionsMessages.h"
#include "TransactionsJavascript.h"
#include "TransactionsDBStorage.h"

#include <memory>

SET_LOG_NAMESPACE("TXS");

namespace transactions {

static const uint64_t ADD_TO_COUNT_TXS = 10;

Transactions::Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, QObject *parent)
    : TimerClass(5s, parent)
    , nsLookup(nsLookup)
    , javascriptWrapper(javascriptWrapper)
    , db(db)
{
    CHECK(connect(this, &Transactions::callbackCall, this, &Transactions::onCallbackCall), "not connect onCallbackCall");

    CHECK(connect(this, &Transactions::timerEvent, this, &Transactions::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &Transactions::startedEvent, this, &Transactions::onRun), "not connect run");

    CHECK(connect(this, &Transactions::registerAddresses, this, &Transactions::onRegisterAddresses), "not connect onRegisterAddresses");
    CHECK(connect(this, &Transactions::getAddresses, this, &Transactions::onGetAddresses), "not connect onGetAddresses");
    CHECK(connect(this, &Transactions::setCurrentGroup, this, &Transactions::onSetCurrentGroup), "not connect onSetCurrentGroup");
    CHECK(connect(this, &Transactions::getTxs, this, &Transactions::onGetTxs), "not connect onGetTxs");
    CHECK(connect(this, &Transactions::getTxs2, this, &Transactions::onGetTxs2), "not connect onGetTxs2");
    CHECK(connect(this, &Transactions::getTxsAll, this, &Transactions::onGetTxsAll), "not connect onGetTxsAll");
    CHECK(connect(this, &Transactions::getTxsAll2, this, &Transactions::onGetTxsAll2), "not connect onGetTxsAll2");
    CHECK(connect(this, &Transactions::getForgingTxs, this, &Transactions::onGetForgingTxs), "not connect onGetForgingTxs");
    CHECK(connect(this, &Transactions::getLastForgingTx, this, &Transactions::onGetLastForgingTx), "not connect onGetLastForgingTx");
    CHECK(connect(this, &Transactions::calcBalance, this, &Transactions::onCalcBalance), "not connect onCalcBalance");
    CHECK(connect(this, &Transactions::sendTransaction, this, &Transactions::onSendTransaction), "not connect onSendTransaction");
    CHECK(connect(this, &Transactions::getTxFromServer, this, &Transactions::onGetTxFromServer), "not connect onGetTxFromServer");
    CHECK(connect(this, &Transactions::getLastUpdateBalance, this, &Transactions::onGetLastUpdateBalance), "not connect onGetLastUpdateBalance");
    CHECK(connect(this, &Transactions::getNonce, this, &Transactions::onGetNonce), "not connect onGetNonce");
    CHECK(connect(this, &Transactions::getDelegateStatus, this, &Transactions::onGetDelegateStatus), "not connect onGetDelegateStatus");
    CHECK(connect(this, &Transactions::clearDb, this, &Transactions::onClearDb), "not connect onClearDb");

    Q_REG(Transactions::Callback, "Transactions::Callback");
    Q_REG(RegisterAddressCallback, "RegisterAddressCallback");
    Q_REG(GetTxsCallback, "GetTxsCallback");
    Q_REG(CalcBalanceCallback, "CalcBalanceCallback");
    Q_REG(SetCurrentGroupCallback, "SetCurrentGroupCallback");
    Q_REG(GetAddressesCallback, "GetAddressesCallback");
    Q_REG(GetTxCallback, "GetTxCallback");
    Q_REG(GetLastUpdateCallback, "GetLastUpdateCallback");
    Q_REG(GetNonceCallback, "GetNonceCallback");
    Q_REG(GetStatusDelegateCallback, "GetStatusDelegateCallback");
    Q_REG(SendTransactionCallback, "SendTransactionCallback");
    Q_REG(ClearDbCallback, "ClearDbCallback");
    Q_REG(SignalFunc, "SignalFunc");

    Q_REG2(size_t, "size_t", false);
    Q_REG2(seconds, "seconds", false);
    Q_REG(DelegateStatus, "DelegateStatus");
    Q_REG(SendParameters, "SendParameters");

    Q_REG(std::vector<AddressInfo>, "std::vector<AddressInfo>");

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("timeouts_sec/transactions"), "settings timeout not found");
    timeout = seconds(settings.value("timeouts_sec/transactions").toInt());

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Transactions::callbackCall), "not connect callbackCall");
    client.moveToThread(&thread1);

    CHECK(connect(&tcpClient, &HttpSimpleClient::callbackCall, this, &Transactions::callbackCall), "not connect callbackCall");
    tcpClient.moveToThread(&thread1);

    timerSendTx.moveToThread(&thread1);
    timerSendTx.setInterval(milliseconds(100).count());
    CHECK(connect(&timerSendTx, &QTimer::timeout, this, &Transactions::onFindTxOnTorrentEvent), "not connect onFindTxOnTorrentEvent");
    CHECK(connect(&thread1, &QThread::finished, &timerSendTx, &QTimer::stop), "not connect stop");

    javascriptWrapper.setTransactions(*this);

    moveToThread(&thread1); // TODO вызывать в TimerClass
}

void Transactions::onCallbackCall(Callback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void Transactions::onRun() {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
}

template<typename Func>
void Transactions::runCallback(const Func &callback) {
    emit javascriptWrapper.callbackCall(callback);
}

uint64_t Transactions::calcCountTxs(const QString &address, const QString &currency) const {
    const uint64_t countReceived = static_cast<uint64_t>(db.getPaymentsCountForAddress(address, currency, false));
    const uint64_t countSpent = static_cast<uint64_t>(db.getPaymentsCountForAddress(address, currency, true));
    return  countReceived + countSpent;
}

void Transactions::newBalance(const QString &address, const QString &currency, uint64_t savedCountTxs, const BalanceInfo &balance, const std::vector<Transaction> &txs, const std::shared_ptr<ServersStruct> &servStruct) {
    const uint64_t currCountTxs = calcCountTxs(address, currency);
    CHECK(savedCountTxs == currCountTxs, "Trancastions in db on address " + address.toStdString() + " " + currency.toStdString() + " changed");
    auto transactionGuard = db.beginTransaction();
    for (const Transaction &tx: txs) {
        db.addPayment(tx);
    }
    transactionGuard.commit();
    emit javascriptWrapper.newBalanceSig(address, currency, balance);
    updateBalanceTime(currency, servStruct);
}

void Transactions::updateBalanceTime(const QString &currency, const std::shared_ptr<ServersStruct> &servStruct) {
    CHECK(servStruct != nullptr, "Incorrect servStruct");
    CHECK(servStruct->currency == currency, "Incorrect servStruct currency");
    servStruct->countRequests--;
    if (servStruct->countRequests == 0) {
        const system_time_point now = ::system_now();
        lastSuccessUpdateTimestamps[currency] = now;
    }
}

void Transactions::processPendingsMth(const std::vector<QString> &servers) {
    if (servers.empty()) {
        return;
    }

    const auto processPendingTx = [this](const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const Transaction tx = parseGetTxResponse(QString::fromStdString(response), "", "");
        if (tx.status != Transaction::PENDING) {
            if (std::find(pendingTxsAfterSend.begin(), pendingTxsAfterSend.end(), tx.tx) != pendingTxsAfterSend.end()) {
                pendingTxsAfterSend.erase(std::remove(pendingTxsAfterSend.begin(), pendingTxsAfterSend.end(), tx.tx), pendingTxsAfterSend.end());
                emit javascriptWrapper.transactionStatusChanged2Sig(tx.tx, tx);
            }
        }
    };

    LOG << PeriodicLog::make("pas") << "Pending after send: " << pendingTxsAfterSend.size();

    const auto copyPending = pendingTxsAfterSend;
    for (const QString &txHash: copyPending) {
        const QString message = makeGetTxRequest(txHash);
        for (const QString &server: servers) {
            client.sendMessagePost(server, message, processPendingTx, timeout);
        }
    }
}

void Transactions::processAddressMth(const QString &address, const QString &currency, const std::vector<QString> &servers, const std::shared_ptr<ServersStruct> &servStruct, const std::vector<QString> &pendingTxs) {
    if (servers.empty()) {
        return;
    }

    const auto processPendingTx = [this, address, currency](const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const Transaction tx = parseGetTxResponse(QString::fromStdString(response), address, currency);
        if (tx.status != Transaction::PENDING) {
            db.updatePayment(address, currency, tx.tx, tx.isInput, tx);
            emit javascriptWrapper.transactionStatusChangedSig(address, currency, tx.tx, tx);
            emit javascriptWrapper.transactionStatusChanged2Sig(tx.tx, tx);
        }
    };

    const auto getBlockHeaderCallback = [this, address, currency, servStruct](const BalanceInfo &balance, uint64_t savedCountTxs, std::vector<Transaction> txs, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const BlockInfo bi = parseGetBlockInfoResponse(QString::fromStdString(response));
        for (Transaction &tx: txs) {
            if (tx.blockNumber == bi.number) {
                tx.blockHash = bi.hash;
            }
        }
        newBalance(address, currency, savedCountTxs, balance, txs, servStruct);
    };

    const auto processNewTransactions = [this, address, currency, servStruct, getBlockHeaderCallback](const BalanceInfo &balance, uint64_t savedCountTxs, const std::vector<Transaction> &txs, const QUrl &server) {
        const auto maxElement = std::max_element(txs.begin(), txs.end(), [](const Transaction &first, const Transaction &second) {
            return first.blockNumber < second.blockNumber;
        });
        CHECK(maxElement != txs.end(), "Incorrect min element");
        const int64_t blockNumber = maxElement->blockNumber;
        const QString request = makeGetBlockInfoRequest(blockNumber);
        client.sendMessagePost(server, request, std::bind(getBlockHeaderCallback, balance, savedCountTxs, txs, _1, _2), timeout);
    };

    const auto getAllHistoryCallback = [address, currency, servStruct, processNewTransactions](const BalanceInfo &balance, uint64_t savedCountTxs, const QUrl &server, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const std::vector<Transaction> txs = parseHistoryResponse(address, currency, QString::fromStdString(response));

        LOG << "Txs geted2 " << address << " " << txs.size();
        processNewTransactions(balance, savedCountTxs, txs, server);
    };

    const auto getBalanceConfirmeCallback = [this, address, currency, getAllHistoryCallback, processNewTransactions, servStruct](const BalanceInfo &serverBalance, uint64_t savedCountTxs, const std::vector<Transaction> &txs, const QUrl &server, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const BalanceInfo balance = parseBalanceResponse(QString::fromStdString(response));
        const uint64_t countInServer = balance.countReceived + balance.countSpent;
        const uint64_t countSave = serverBalance.countReceived + serverBalance.countSpent;
        if (countInServer - countSave <= ADD_TO_COUNT_TXS) {
            LOG << "Balance " << address << " confirmed";
            processNewTransactions(serverBalance, savedCountTxs, txs, server);
        } else {
            LOG << "Balance " << address << " not confirmed";
            const QString requestForTxs = makeGetHistoryRequest(address, false, 0);

            client.sendMessagePost(server, requestForTxs, std::bind(getAllHistoryCallback, balance, savedCountTxs, server, _1, _2), timeout);
        }
    };

    const auto getHistoryCallback = [this, address, currency, getAllHistoryCallback, getBalanceConfirmeCallback](const BalanceInfo &serverBalance, uint64_t savedCountTxs, const QUrl &server, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const std::vector<Transaction> txs = parseHistoryResponse(address, currency, QString::fromStdString(response));

        LOG << "Txs geted " << address << " " << txs.size();

        const QString requestBalance = makeGetBalanceRequest(address);

        client.sendMessagePost(server, requestBalance, std::bind(getBalanceConfirmeCallback, serverBalance, savedCountTxs, txs, server, _1, _2), timeout);
    };

    const auto getBalanceCallback = [this, servStruct, address, currency, getAllHistoryCallback, getBalanceConfirmeCallback, getHistoryCallback, pendingTxs, processPendingTx](const std::vector<QUrl> &servers, const std::vector<std::tuple<std::string, SimpleClient::ServerException>> &responses) {
        CHECK(!servers.empty(), "Incorrect response size");
        CHECK(servers.size() == responses.size(), "Incorrect response size");
        QUrl bestServer;
        BalanceInfo serverBalance;
        for (size_t i = 0; i < responses.size(); i++) {
            const auto &responsePair = responses[i];
            const auto &exception = std::get<SimpleClient::ServerException>(responsePair);
            const std::string &response = std::get<std::string>(responsePair);
            const QUrl &server = servers[i];
            if (!exception.isSet()) {
                const BalanceInfo balanceResponse = parseBalanceResponse(QString::fromStdString(response));
                CHECK(balanceResponse.address == address, "Incorrect response: address not equal. Expected " + address.toStdString() + ". Received " + balanceResponse.address.toStdString());
                if (balanceResponse.currBlockNum > serverBalance.currBlockNum) {
                    serverBalance = balanceResponse;
                    bestServer = server;
                }
            }
        }

        CHECK(!bestServer.isEmpty(), "Best server with txs not found. Error: " + std::get<SimpleClient::ServerException>(responses[0]).toString());
        const uint64_t countAll = calcCountTxs(address, currency);
        const uint64_t countInServer = serverBalance.countReceived + serverBalance.countSpent;
        LOG << PeriodicLog::make("t_" + address.right(4).toStdString()) << "Automatic get txs " << address << " " << currency << " " << countAll << " " << countInServer;
        if (countAll < countInServer) {
            processCheckTxsOneServer(address, currency, bestServer);

            const uint64_t countMissingTxs = countInServer - countAll;
            const uint64_t requestCountTxs = countMissingTxs + ADD_TO_COUNT_TXS;
            const QString requestForTxs = makeGetHistoryRequest(address, true, requestCountTxs);

            client.sendMessagePost(bestServer, requestForTxs, std::bind(getHistoryCallback, serverBalance, countAll, bestServer, _1, _2), timeout);
        } else {
            updateBalanceTime(currency, servStruct);
        }

        if (!pendingTxs.empty()) {
            LOG << PeriodicLog::make("pt_" + address.right(4).toStdString()) << "Pending txs: " << pendingTxs.size();
        }

        for (const QString &txHash: pendingTxs) {
            const QString message = makeGetTxRequest(txHash);
            client.sendMessagePost(bestServer, message, processPendingTx, timeout);
        }
    };

    const QString requestBalance = makeGetBalanceRequest(address);
    const std::vector<QUrl> urls(servers.begin(), servers.end());
    client.sendMessagesPost(address.toStdString(), urls, requestBalance, std::bind(getBalanceCallback, urls, _1), timeout);
}

std::vector<AddressInfo> Transactions::getAddressesInfos(const QString &group) {
    return db.getTrackedForGroup(group);
}

BalanceInfo Transactions::getBalance(const QString &address, const QString &currency) {
    BalanceInfo balance;
    db.calcBalance(address, currency, balance);

    balance.received += balance.undelegate;
    balance.spent += balance.delegate;

    return balance;
}

void Transactions::processCheckTxsOneServer(const QString &address, const QString &currency, const QUrl &server) {
    const Transaction lastTx = db.getLastTransaction(address, currency);
    if (lastTx.blockHash.isEmpty()) {
        return;
    }

    const auto countBlocksCallback = [this, address, currency, lastTx](const QUrl &server, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server exception: " + exception.toString());

        const int64_t blockNumber = parseGetCountBlocksResponse(QString::fromStdString(response));

        processCheckTxsInternal(address, currency, server, lastTx, blockNumber);
    };

    const QString countBlocksRequest = makeGetCountBlocksRequest();
    client.sendMessagePost(server, countBlocksRequest, std::bind(countBlocksCallback, server, _1, _2), timeout);
}

void Transactions::processCheckTxsInternal(const QString &address, const QString &currency, const QUrl &server, const Transaction &tx, int64_t serverBlockNumber) {
    const auto removeAllTxs = [this](const QString &address, const QString &currency) {
        LOG << "Remove txs " << address << " " << currency;
        db.removePaymentsForDest(address, currency);
    };

    const auto getBlockInfoCallback = [address, currency, removeAllTxs] (const QString &hash, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const BlockInfo bi = parseGetBlockInfoResponse(QString::fromStdString(response));
        if (bi.hash != hash) {
            removeAllTxs(address, currency);
        }
    };

    if (tx.blockNumber > serverBlockNumber) {
        removeAllTxs(address, currency);
        return;
    }

    const QString blockInfoRequest = makeGetBlockInfoRequest(tx.blockNumber);
    client.sendMessagePost(server, blockInfoRequest, std::bind(getBlockInfoCallback, tx.blockHash, _1, _2));
}

void Transactions::processCheckTxs(const QString &address, const QString &currency, const std::vector<QString> &servers) {
    if (servers.empty()) {
        return;
    }
    const Transaction lastTx = db.getLastTransaction(address, currency);
    if (lastTx.blockHash.isEmpty()) {
        return;
    }

    const auto countBlocksCallback = [this, address, currency, lastTx](const std::vector<QUrl> &urls, const std::vector<std::tuple<std::string, SimpleClient::ServerException>> &responses) {
        CHECK(!urls.empty(), "Incorrect response size");
        CHECK(urls.size() == responses.size(), "Incorrect responses size");

        int64_t maxBlockNumber = 0;
        QUrl maxServer;
        for (size_t i = 0; i < responses.size(); i++) {
            const SimpleClient::ServerException &exception = std::get<SimpleClient::ServerException>(responses[i]);
            if (!exception.isSet()) {
                const QUrl &server = urls[i];
                const std::string &response = std::get<std::string>(responses[i]);
                const int64_t blockNumber = parseGetCountBlocksResponse(QString::fromStdString(response));
                if (blockNumber > maxBlockNumber) {
                    maxBlockNumber = blockNumber;
                    maxServer = server;
                }
            }
        }
        CHECK(!maxServer.isEmpty(), "Best server with txs not found. Error: " + std::get<SimpleClient::ServerException>(responses[0]).toString());
        processCheckTxsInternal(address, currency, maxServer, lastTx, maxBlockNumber);
    };

    const QString countBlocksRequest = makeGetCountBlocksRequest();
    const std::vector<QUrl> urls(servers.begin(), servers.end());
    client.sendMessagesPost(address.toStdString(), urls, countBlocksRequest, std::bind(countBlocksCallback, urls, _1), timeout);
}

void Transactions::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    static const size_t MAXIMUM_ADDRESSES = 20;

    if (posInAddressInfos >= addressesInfos.size()) {
        if (!addressesInfos.empty()) {
            LOG << "All txs getted";
        }
        addressesInfos.clear();
    }

    if (addressesInfos.empty()) {
        addressesInfos = getAddressesInfos(currentGroup);
        std::sort(addressesInfos.begin(), addressesInfos.end(), [](const AddressInfo &first, const AddressInfo &second) {
            return first.type < second.type;
        });
        posInAddressInfos = 0;
    }
    LOG << PeriodicLog::make("f_bln") << "Try fetch balance " << addressesInfos.size();
    std::vector<QString> servers;
    QString currentType;
    std::map<QString, std::shared_ptr<ServersStruct>> servStructs;
    const time_point now = ::now();
    const auto checkTxsPeriod = 3min;
    for (size_t i = posInAddressInfos; i < std::min(addressesInfos.size(), posInAddressInfos + MAXIMUM_ADDRESSES); i++) {
        const AddressInfo &addr = addressesInfos[i];
        if (addr.type != currentType) {
            servers = nsLookup.getRandom(addr.type, 3, 3);
            if (servers.empty()) {
                LOG << "Warn: servers empty: " << addr.type;
                continue;
            }
            currentType = addr.type;
        }
        const auto found = servStructs.find(addr.currency);
        if (found == servStructs.end()) {
            servStructs.emplace(std::piecewise_construct, std::forward_as_tuple(addr.currency), std::forward_as_tuple(std::make_shared<ServersStruct>(addr.currency)));
        }
        servStructs.at(addr.currency)->countRequests++; // Не очень хорошо здесь прибавлять по 1, но пофиг
        if (now - lastCheckTxsTime >= checkTxsPeriod) {
            processCheckTxs(addr.address, addr.currency, servers);
        }
        const std::vector<Transaction> pendingTxs = db.getPaymentsForAddressPending(addr.address, addr.currency, true);
        std::vector<QString> pendingTxsStrs;
        pendingTxsStrs.reserve(pendingTxs.size());
        std::transform(pendingTxs.begin(), pendingTxs.end(), std::back_inserter(pendingTxsStrs), [](const Transaction &tx) { return tx.tx;});
        processAddressMth(addr.address, addr.currency, servers, servStructs.at(addr.currency), pendingTxsStrs);
    }
    posInAddressInfos += MAXIMUM_ADDRESSES;

    if (now - lastCheckTxsTime >= checkTxsPeriod && posInAddressInfos >= addressesInfos.size()) {
        LOG << "All txs checked";
        lastCheckTxsTime = now;
    }

    processPendingsMth(servers);
END_SLOT_WRAPPER
}

void Transactions::onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        auto transactionGuard = db.beginTransaction();
        for (const AddressInfo &address: addresses) {
            db.addTracked(address);
        }
        transactionGuard.commit();
    });
    runCallback(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetAddresses(const QString &group, const GetAddressesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<AddressInfo> result;
    const TypedException exception = apiVrapper2([&, this] {
        result = getAddressesInfos(group);
        for (AddressInfo &info: result) {
            info.balance = getBalance(info.address, info.currency);
        }
    });
    runCallback(std::bind(callback, result, exception));
END_SLOT_WRAPPER
}

void Transactions::onSetCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback) {
BEGIN_SLOT_WRAPPER
    currentGroup = group;
    runCallback(std::bind(callback, TypedException()));
END_SLOT_WRAPPER
}

void Transactions::onGetTxs(const QString &address, const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    // TODO
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetTxs2(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {
        txs = db.getPaymentsForAddress(address, currency, from, count, asc);
    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetTxsAll(const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    // TODO
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetTxsAll2(const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {
        txs = db.getPaymentsForCurrency(currency, from, count, asc);
    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetForgingTxs(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {
        txs = db.getForgingPaymentsForAddress(address, currency, from, count, asc);
    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetLastForgingTx(const QString &address, const QString &currency, const GetTxCallback &callback) {
BEGIN_SLOT_WRAPPER
    Transaction txs;
    const TypedException exception = apiVrapper2([&, this] {
        txs = db.getLastForgingTransaction(address, currency);
    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onCalcBalance(const QString &address, const QString &currency, const CalcBalanceCallback &callback) {
BEGIN_SLOT_WRAPPER
    BalanceInfo balance;
    const TypedException exception = apiVrapper2([&, this] {
        balance = getBalance(address, currency);
    });
    runCallback(std::bind(callback, balance, exception));
END_SLOT_WRAPPER
}

Transactions::SendedTransactionWatcher::~SendedTransactionWatcher() {
    for (const QString &server: allServers) {
        txManager.sendErrorGetTx(requestId, hash, server);
        const auto found = errors.find(server);
        if (found != errors.end()) {
            LOG << "Get tx not parse " << server << " " << hash << " " << found->second;
        }
    }
}

void Transactions::sendErrorGetTx(const QString &requestId, const TransactionHash &hash, const QString &server) {
    emit javascriptWrapper.transactionInTorrentSig(requestId, server, QString::fromStdString(hash), Transaction(), TypedException(TypeErrors::TRANSACTIONS_SENDED_NOT_FOUND, "Transaction not found"));
}

void Transactions::onFindTxOnTorrentEvent() {
BEGIN_SLOT_WRAPPER
    const time_point now = ::now();

    for (auto iter = sendTxWathcers.begin(); iter != sendTxWathcers.end();) {
        const TransactionHash &hash = iter->first;
        SendedTransactionWatcher &watcher = iter->second;

        std::shared_ptr<bool> isFirst = std::make_shared<bool>(true);
        const QString message = makeGetTxRequest(QString::fromStdString(hash));
        const auto serversCopy = watcher.getServersCopy();
        for (const QString &server: serversCopy) {
            // Удаляем, чтобы не заддосить сервер на следующей итерации
            watcher.removeServer(server);
            client.sendMessagePost(server, message, [this, server, hash, isFirst, requestId=watcher.requestId](const std::string &response, const SimpleClient::ServerException &exception) {
                auto found = sendTxWathcers.find(hash);
                if (found == sendTxWathcers.end()) {
                    return;
                }
                if (!exception.isSet()) {
                    try {
                        const Transaction tx = parseGetTxResponse(QString::fromStdString(response), "", "");
                        emit javascriptWrapper.transactionInTorrentSig(requestId, server, QString::fromStdString(hash), tx, TypedException());
                        if (tx.status == Transaction::Status::PENDING) {
                            pendingTxsAfterSend.emplace_back(tx.tx);
                        }
                        if (*isFirst) {
                            *isFirst = false;
                            emit timerEvent();
                        }
                        found->second.okServer(server);
                        return;
                    } catch (const Exception &e) {
                        found->second.setError(server, QString::fromStdString(e));
                    } catch (...) {
                        // empty;
                    }
                }
                found->second.returnServer(server);
            }, timeout);
        }

        if (watcher.isTimeout(now)) {
            iter = sendTxWathcers.erase(iter);
        } else if (watcher.isEmpty()) {
            iter = sendTxWathcers.erase(iter);
        } else {
            iter++;
        }
    }
    if (sendTxWathcers.empty()) {
        LOG << "SendTxWatchers timer send stop";
        timerSendTx.stop();
    }
END_SLOT_WRAPPER
}

void Transactions::addToSendTxWatcher(const QString &requestId, const TransactionHash &hash, size_t countServers, const std::vector<QString> &servers, const seconds &timeout) {
    if (sendTxWathcers.find(hash) != sendTxWathcers.end()) {
        return;
    }

    const size_t remainServersGet = countServers - servers.size();
    for (size_t i = 0; i < remainServersGet; i++) {
        emit javascriptWrapper.transactionInTorrentSig(requestId, "", QString::fromStdString(hash), Transaction(), TypedException(TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "dns return less laid"));
    }
    const time_point now = ::now();
    sendTxWathcers.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(*this, requestId, hash, now, servers, timeout));
    LOG << "SendTxWatchers timer send start";
    timerSendTx.start();
}

void Transactions::onSendTransaction(const QString &requestId, const QString &to, const QString &value, size_t nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign, const SendParameters &sendParams, const SendTransactionCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        const QString request = makeSendTransactionRequest(to, value, nonce, data, fee, pubkey, sign);
        const size_t countServersSend = sendParams.countServersSend;
        const std::vector<QString> servers = nsLookup.getRandom(sendParams.typeSend, countServersSend, countServersSend);
        CHECK_TYPED(!servers.empty(), TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "Not enough servers send");
        const size_t remainServersSend = countServersSend - servers.size();
        for (size_t i = 0; i < remainServersSend; i++) {
            emit javascriptWrapper.sendedTransactionsResponseSig(requestId, "", "", TypedException(TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "dns return less laid"));
        }

        const std::vector<QString> serversGet = nsLookup.getRandom(sendParams.typeGet, sendParams.countServersGet, sendParams.countServersGet); // Получаем список серверов здесь, чтобы здесь же обработать ошибки
        CHECK_TYPED(!serversGet.empty(), TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "Not enough servers get");

        for (const QString &server: servers) {
            tcpClient.sendMessagePost(server, request, [this, server, requestId, sendParams, serversGet](const std::string &response, const TypedException &error) {
                QString result;
                const TypedException exception = apiVrapper2([&] {
                    CHECK_TYPED(!error.isSet(), TypeErrors::TRANSACTIONS_SERVER_SEND_ERROR, error.description + ". " + server.toStdString());
                    result = parseSendTransactionResponse(QString::fromStdString(response));
                    addToSendTxWatcher(requestId, result.toStdString(), sendParams.countServersGet, serversGet, sendParams.timeout);
                });
                emit javascriptWrapper.sendedTransactionsResponseSig(requestId, server, result, exception);
            }, timeout);
        }
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void Transactions::onGetNonce(const QString &requestId, const QString &from, const SendParameters &sendParams, const GetNonceCallback &callback) {
BEGIN_SLOT_WRAPPER
    const std::vector<QString> servers = nsLookup.getRandom(sendParams.typeGet, sendParams.countServersGet, sendParams.countServersGet);
    CHECK(!servers.empty(), "Not enough servers");

    struct NonceStruct {
        bool isSet = false;
        uint64_t nonce = 0;
        size_t count;
        TypedException exception;
        QString serverError;

        NonceStruct(size_t count)
           : count(count)
        {}
    };

    std::shared_ptr<NonceStruct> nonceStruct = std::make_shared<NonceStruct>(servers.size());

    const auto getBalanceCallback = [this, nonceStruct, requestId, from, callback](const QString &server, const std::string &response, const SimpleClient::ServerException &exception) {
        nonceStruct->count--;

        if (!exception.isSet()) {
            const BalanceInfo balanceResponse = parseBalanceResponse(QString::fromStdString(response));
            nonceStruct->isSet = true;
            nonceStruct->nonce = std::max(nonceStruct->nonce, balanceResponse.countSpent);
        } else {
            nonceStruct->exception = TypedException(TypeErrors::CLIENT_ERROR, exception.description);
            nonceStruct->serverError = server;
        }

        if (nonceStruct->count == 0) {
            if (!nonceStruct->isSet) {
                callback.emitFunc(nonceStruct->exception, 0, nonceStruct->serverError);
            } else {
                callback.emitFunc(TypedException(), nonceStruct->nonce + 1, "");
            }
        }
    };

    const QString requestBalance = makeGetBalanceRequest(from);
    for (const QString &server: servers) {
        client.sendMessagePost(server, requestBalance, std::bind(getBalanceCallback, server, _1, _2), timeout);
    }
END_SLOT_WRAPPER
}

void Transactions::onGetTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        const QString message = makeGetTxRequest(txHash);

        const std::vector<QString> servers = nsLookup.getRandom(type, 1, 1);
        CHECK(!servers.empty(), "Not enough servers");
        const QString &server = servers[0];

        client.sendMessagePost(server, message, [this, callback](const std::string &response, const SimpleClient::ServerException &error) mutable {
            Transaction tx;
            const TypedException exception = apiVrapper2([&] {
                CHECK_TYPED(!error.isSet(), TypeErrors::CLIENT_ERROR, error.description);
                tx = parseGetTxResponse(QString::fromStdString(response), "", "");
            });
            runCallback(std::bind(callback, tx, exception));
        }, timeout);
    });

    if (exception.isSet()) {
        runCallback(std::bind(callback, Transaction(), exception));
    }
END_SLOT_WRAPPER
}

void Transactions::onGetLastUpdateBalance(const QString &currency, const GetLastUpdateCallback &callback) {
BEGIN_SLOT_WRAPPER
    const system_time_point now = ::system_now();
    auto found = lastSuccessUpdateTimestamps.find(currency);
    system_time_point result;
    if (found != lastSuccessUpdateTimestamps.end()) {
        result = found->second;
    }
    runCallback(std::bind(callback, result, now));
END_SLOT_WRAPPER
}

void Transactions::onGetDelegateStatus(const QString &address, const QString &currency, const QString &from, const QString &to, bool isInput, const GetStatusDelegateCallback &callback) {
BEGIN_SLOT_WRAPPER
    DelegateStatus status = DelegateStatus::NOT_FOUND;
    Transaction txDelegate;
    Transaction txUnDelegate;
    const TypedException exception = apiVrapper2([&, this] {
        txDelegate = db.getLastPaymentIsSetDelegate(address, currency, from, to, isInput, true);
        txUnDelegate = db.getLastPaymentIsSetDelegate(address, currency, from, to, isInput, false);
        if (txDelegate.id == -1) {
            status = DelegateStatus::NOT_FOUND;
        } else if (txDelegate.status == Transaction::PENDING) {
            status = DelegateStatus::PENDING;
        } else if (txDelegate.status == Transaction::ERROR) {
            status = DelegateStatus::ERROR;
        } else if (txUnDelegate.id != -1 && txUnDelegate.timestamp > txDelegate.timestamp) {
            status = DelegateStatus::UNDELEGATE;
        } else {
            status = DelegateStatus::DELEGATE;
        }
    });
    runCallback(std::bind(callback, exception, status, txDelegate, txUnDelegate));
END_SLOT_WRAPPER
}

void Transactions::onClearDb(const QString &currency, const ClearDbCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        db.removePaymentsForCurrency(currency);
        nsLookup.resetFile();
    });
    runCallback(std::bind(callback, exception));
END_SLOT_WRAPPER
}

SendParameters parseSendParams(const QString &paramsJson) {
    return parseSendParamsInternal(paramsJson);
}

}
