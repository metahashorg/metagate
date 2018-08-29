#include "Transactions.h"

#include <functional>
using namespace std::placeholders;

#include "check.h"
#include "SlotWrapper.h"

#include "NsLookup.h"

#include "TransactionsMessages.h"
#include "TransactionsJavascript.h"
#include "transactionsdbstorage.h"

#include <memory>

namespace transactions {

static const uint64_t ADD_TO_COUNT_TXS = 10;

Transactions::Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, QObject *parent)
    : TimerClass(2s, parent)
    , nsLookup(nsLookup)
    , javascriptWrapper(javascriptWrapper)
    , db(db)
{
    CHECK(connect(this, &Transactions::timerEvent, this, &Transactions::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &Transactions::startedEvent, this, &Transactions::onRun), "not connect run");

    CHECK(connect(this, &Transactions::registerAddresses, this, &Transactions::onRegisterAddresses), "not connect onRegisterAddresses");
    CHECK(connect(this, &Transactions::getAddresses, this, &Transactions::onGetAddresses), "not connect onGetAddresses");
    CHECK(connect(this, &Transactions::setCurrentGroup, this, &Transactions::onSetCurrentGroup), "not connect onSetCurrentGroup");
    CHECK(connect(this, &Transactions::getTxs, this, &Transactions::onGetTxs), "not connect onGetTxs");
    CHECK(connect(this, &Transactions::getTxs2, this, &Transactions::onGetTxs2), "not connect onGetTxs2");
    CHECK(connect(this, &Transactions::getTxsAll, this, &Transactions::onGetTxsAll), "not connect onGetTxsAll");
    CHECK(connect(this, &Transactions::getTxsAll2, this, &Transactions::onGetTxsAll2), "not connect onGetTxsAll2");
    CHECK(connect(this, &Transactions::calcBalance, this, &Transactions::onCalcBalance), "not connect onCalcBalance");
    CHECK(connect(this, &Transactions::sendTransaction, this, &Transactions::onSendTransaction), "not connect onSendTransaction");

    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<RegisterAddressCallback>("RegisterAddressCallback");
    qRegisterMetaType<GetTxsCallback>("GetTxsCallback");
    qRegisterMetaType<CalcBalanceCallback>("CalcBalanceCallback");
    qRegisterMetaType<SetCurrentGroupCallback>("SetCurrentGroupCallback");
    qRegisterMetaType<SetCurrentGroupCallback>("SetCurrentGroupCallback");
    qRegisterMetaType<GetAddressesCallback>("GetAddressesCallback");
    qRegisterMetaType<GetTxCallback>("GetAddressesCallback");

    qRegisterMetaType<std::vector<AddressInfo>>("std::vector<AddressInfo>");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Transactions::onCallbackCall), "not connect");
    client.moveToThread(&thread1);

    tcpClient.setParent(this);
    CHECK(connect(&tcpClient, &HttpSimpleClient::callbackCall, this, &Transactions::onCallbackCall), "not connect");
    tcpClient.moveToThread(&thread1);

    timerSendTx.moveToThread(&thread1);
    timerSendTx.setInterval(milliseconds(100).count()); // TODO сделать так, чтобы таймер запускался только когда нужно, а не постоянно чекал событие
    CHECK(connect(&timerSendTx, SIGNAL(timeout()), this, SLOT(onSendTxEvent())), "not connect");
    CHECK(timerSendTx.connect(&thread1, SIGNAL(started()), SLOT(start())), "not connect");
    CHECK(timerSendTx.connect(&thread1, SIGNAL(finished()), SLOT(stop())), "not connect");

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

struct BalanceStruct {
    QString address;
    size_t countResponses = 0;
    BalanceInfo balance;

    QString server;

    BalanceStruct(const QString &address)
        : address(address)
    {}
};

void Transactions::newBalance(const QString &address, const QString &currency, const BalanceInfo &balance, const std::vector<Transaction> &txs) {
    for (Transaction tx: txs) {
        tx.address = address;
        tx.currency = currency;
        db.addPayment(tx);
    }
    emit javascriptWrapper.newBalanceSig(address, currency, balance);
}

void Transactions::processAddressMth(const QString &address, const QString &currency, const std::vector<QString> &servers) {
    if (servers.empty()) {
        return;
    }

    std::shared_ptr<BalanceStruct> balanceStruct = std::make_shared<BalanceStruct>(address);

    const auto getAllHistoryCallback = [this, address, currency](const BalanceInfo &balance, const std::string &response, const TypedException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.description);
        const std::vector<Transaction> txs = parseHistoryResponse(address, QString::fromStdString(response));

        LOG << "Txs geted2 " << address << " " << txs.size();
        newBalance(address, currency, balance, txs);
    };

    const auto getBalanceConfirmeCallback = [this, balanceStruct, currency, getAllHistoryCallback](const std::vector<Transaction> &txs, const QString &server, const std::string &response, const TypedException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.description);
        const BalanceInfo balance = parseBalanceResponse(QString::fromStdString(response));
        const uint64_t countInServer = balance.countReceived + balance.countSpent;
        const uint64_t countSave = balanceStruct->balance.countReceived + balanceStruct->balance.countSpent;
        if (countInServer - countSave <= ADD_TO_COUNT_TXS) {
            LOG << "Balance " << balanceStruct->address << " confirmed";
            newBalance(balanceStruct->address, currency, balanceStruct->balance, txs);
        } else {
            LOG << "Balance " << balanceStruct->address << " not confirmed";
            const QString requestForTxs = makeGetHistoryRequest(balanceStruct->address, false, 0);

            client.sendMessagePost(server, requestForTxs, std::bind(getAllHistoryCallback, balance, _1, _2), 1s);
        }
    };

    const auto getHistoryCallback = [this, address, getAllHistoryCallback, getBalanceConfirmeCallback](const QString &server, const std::string &response, const TypedException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.description);
        const std::vector<Transaction> txs = parseHistoryResponse(address, QString::fromStdString(response));

        LOG << "Txs geted " << address << " " << txs.size();

        const QString requestBalance = makeGetBalanceRequest(address);

        client.sendMessagePost(server, requestBalance, std::bind(getBalanceConfirmeCallback, txs, server, _1, _2), 1s);
    };

    const auto getBalanceCallback = [this, balanceStruct, currency, getAllHistoryCallback, getBalanceConfirmeCallback, getHistoryCallback](const QString &server, const std::string &response, const TypedException &exception) {
        balanceStruct->countResponses--;

        if (!exception.isSet()) {
            const BalanceInfo balanceResponse = parseBalanceResponse(QString::fromStdString(response));
            CHECK(balanceResponse.address == balanceStruct->address, "Incorrect response: address not equal. Expected " + balanceStruct->address.toStdString() + ". Received " + balanceResponse.address.toStdString());
            if (balanceResponse.currBlockNum > balanceStruct->balance.currBlockNum) {
                balanceStruct->balance = balanceResponse;
                balanceStruct->server = server;
            }
        }

        if (balanceStruct->countResponses == 0 && !balanceStruct->server.isEmpty()) {
            const uint64_t countReceived = db.getPaymentsCountForAddress(balanceStruct->address, currency, false);
            const uint64_t countSpent = db.getPaymentsCountForAddress(balanceStruct->address, currency, true);
            const uint64_t countAll = countReceived + countSpent;
            const uint64_t countInServer = balanceStruct->balance.countReceived + balanceStruct->balance.countSpent;
            LOG << "Automatic get txs " << balanceStruct->address << " " << countAll << " " << countInServer;
            if (countAll < countInServer) {
                const uint64_t countMissingTxs = countInServer - countAll;
                const uint64_t requestCountTxs = countMissingTxs + ADD_TO_COUNT_TXS;
                const QString requestForTxs = makeGetHistoryRequest(balanceStruct->address, true, requestCountTxs);

                client.sendMessagePost(server, requestForTxs, std::bind(getHistoryCallback, server, _1, _2), 1s);
            }
        }
    };

    balanceStruct->countResponses = servers.size();
    for (const QString &server: servers) {
        const QString requestBalance = makeGetBalanceRequest(address);
        client.sendMessagePost(server, requestBalance, std::bind(getBalanceCallback, server, _1, _2), 1s);
    }
}

std::vector<AddressInfo> Transactions::getAddressesInfos(const QString &group) {
    const std::list<AddressInfo> res = db.getTrackedForGroup(group);
    return std::vector<AddressInfo>(res.cbegin(), res.cend());
}

void Transactions::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    std::vector<AddressInfo> addressesInfos = getAddressesInfos(currentGroup);
    std::sort(addressesInfos.begin(), addressesInfos.end(), [](const AddressInfo &first, const AddressInfo &second) {
        return first.type < second.type;
    });
    LOG << "Try fetch balance " << addressesInfos.size();
    std::vector<QString> servers;
    QString currentType;
    for (const AddressInfo &addr: addressesInfos) {
        if (addr.type != currentType) {
            servers = nsLookup.getRandom(addr.type, 3, 3);
            currentType = addr.type;
        }
        processAddressMth(addr.address, addr.currency, servers);
    }
END_SLOT_WRAPPER
}

void Transactions::onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        for (const AddressInfo &address: addresses) {
            db.addTracked(address);
        }
    });
    runCallback(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetAddresses(const QString &group, const GetAddressesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<AddressInfo> result;
    const TypedException exception = apiVrapper2([&, this] {
        result = getAddressesInfos(group);
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
        const std::list<Transaction> result = db.getPaymentsForAddress(address, currency, from, count, asc);
        std::copy(result.cbegin(), result.cend(), std::back_inserter(txs));
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
        const std::list<Transaction> result = db.getPaymentsForCurrency(currency, from, count, asc);
        std::copy(result.cbegin(), result.cend(), std::back_inserter(txs));
    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onCalcBalance(const QString &address, const QString &currency, const CalcBalanceCallback &callback) {
BEGIN_SLOT_WRAPPER
    BalanceInfo balance;
    const TypedException exception = apiVrapper2([&, this] {
        balance.countReceived = db.getPaymentsCountForAddress(address, currency, false);
        balance.countSpent = db.getPaymentsCountForAddress(address, currency, true);
        balance.received = db.calcOutValueForAddress(address, currency).getDecimal();
        balance.spent = db.calcInValueForAddress(address, currency).getDecimal();
    });
    runCallback(std::bind(callback, balance, exception));
END_SLOT_WRAPPER
}

Transactions::SendedTransactionWatcher::~SendedTransactionWatcher() {
    for (const QString &server: allServers) {
        txManager.sendErrorGetTx(hash, server);
        const auto found = errors.find(server);
        if (found != errors.end()) {
            LOG << "Get tx not parse " << server << " " << hash << " " << found->second;
        }
    }
}

void Transactions::sendErrorGetTx(const TransactionHash &hash, const QString &server) {
    emit javascriptWrapper.transactionInTorrentSig(server, QString::fromStdString(hash), Transaction(), TypedException(TypeErrors::TRANSACTIONS_SENDED_NOT_FOUND, "Transaction not found"));
}

void Transactions::onSendTxEvent() {
BEGIN_SLOT_WRAPPER
    const time_point now = ::now();

    for (auto iter = sendTxWathcers.begin(); iter != sendTxWathcers.end();) {
        const TransactionHash &hash = iter->first;
        SendedTransactionWatcher &watcher = iter->second;

        const QString message = makeGetTxRequest(QString::fromStdString(hash));
        const auto serversCopy = watcher.getServersCopy();
        for (const QString &server: serversCopy) {
            // Удаляем, чтобы не заддосить сервер на следующей итерации
            watcher.removeServer(server);
            client.sendMessagePost(server, message, [this, server, hash](const std::string &response, const TypedException &exception) {
                auto found = sendTxWathcers.find(hash);
                if (found == sendTxWathcers.end()) {
                    return;
                }
                if (!exception.isSet()) {
                    try {
                        const Transaction tx = parseGetTxResponse(QString::fromStdString(response));
                        emit javascriptWrapper.transactionInTorrentSig(server, QString::fromStdString(hash), tx, TypedException());
                        found->second.okServer(server);
                        return;
                    } catch (const Exception &e) {
                        found->second.setError(server, QString::fromStdString(e));
                    } catch (...) {
                        // empty;
                    }
                }
                found->second.returnServer(server);
            });
        }

        if (now - watcher.startTime >= seconds(5)) {
            iter = sendTxWathcers.erase(iter);
        } else if (watcher.isEmpty()) {
            iter = sendTxWathcers.erase(iter);
        } else {
            iter++;
        }
    }
END_SLOT_WRAPPER
}

void Transactions::addToSendTxWatcher(const TransactionHash &hash, size_t countServers, const QString &group) {
    if (sendTxWathcers.find(hash) != sendTxWathcers.end()) {
        return;
    }

    const time_point now = ::now();
    sendTxWathcers.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(*this, hash, now, nsLookup.getRandom(group, countServers, countServers)));
}

void Transactions::onSendTransaction(const QString &requestId, int countServersSend, int countServersGet, const QString &to, const QString &value, const QString &nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign, const QString &typeSend, const QString &typeGet) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        const QString request = makeSendTransactionRequest(to, value, nonce, data, fee, pubkey, sign);
        const std::vector<QString> servers = nsLookup.getRandom(typeSend, countServersSend, countServersSend);

        struct ServerResponse {
            int countServers;

            ServerResponse(int countServers)
                : countServers(countServers)
            {}
        };

        std::shared_ptr<ServerResponse> servResp = std::make_shared<ServerResponse>(servers.size());
        for (const QString &server: servers) {
            tcpClient.sendMessagePost(server, request, [this, servResp, server, requestId, countServersGet, typeGet](const std::string &response) {
                servResp->countServers--;
                QString result;
                const TypedException exception = apiVrapper2([&] {
                    CHECK_TYPED(response != HttpSimpleClient::ERROR_BAD_REQUEST, TypeErrors::TRANSACTIONS_SERVER_SEND_ERROR, "Error");
                    result = parseSendTransactionResponse(QString::fromStdString(response));
                });

                if (!exception.isSet()) {
                    addToSendTxWatcher(result.toStdString(), countServersGet, typeGet);
                }
                emit javascriptWrapper.sendedTransactionsResponseSig(requestId, server, result, exception);
            });
        }
    });
END_SLOT_WRAPPER
}

void Transactions::onGetTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        const QString message = makeGetTxRequest(txHash);

        const std::vector<QString> servers = nsLookup.getRandom(type, 1, 1);
        CHECK(servers.size() == 1, "Incorrect servers");
        const QString &server = servers[0];

        client.sendMessagePost(server, message, [this, callback](const std::string &response, const TypedException &error) mutable {
            Transaction tx;
            const TypedException exception = apiVrapper2([&, this] {
                CHECK_TYPED(!error.isSet(), error.numError, error.description);
                tx = parseGetTxResponse(QString::fromStdString(response));
            });
            runCallback(std::bind(callback, tx, exception));
        });
    });

    if (exception.isSet()) {
        runCallback(std::bind(callback, Transaction(), exception));
    }
END_SLOT_WRAPPER
}

}
