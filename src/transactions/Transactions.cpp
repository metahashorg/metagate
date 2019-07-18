#include "Transactions.h"

#include <functional>
using namespace std::placeholders;

#include <QSettings>

#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "Paths.h"
#include "qt_utilites/QRegister.h"

#include "mainwindow.h"
#include "auth/Auth.h"
#include "JavascriptWrapper.h"
#include "Wallet.h"
#include "NsLookup/NsLookup.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "TransactionsMessages.h"
#include "TransactionsJavascript.h"
#include "TransactionsDBStorage.h"

#include <memory>

SET_LOG_NAMESPACE("TXS");

namespace transactions {

static const uint64_t ADD_TO_COUNT_TXS = 10;
static const uint64_t MAX_TXS_IN_RESPONSE = 2000;

// Small hack to convert different currencies names
QString Transactions::convertCurrency(const QString &currency) const
{

    if (currency.contains(QStringLiteral("mhc"), Qt::CaseInsensitive))
        return QStringLiteral("mhc");
    return QStringLiteral("tmh");
}

Transactions::Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, auth::Auth &authManager, MainWindow &mainWin, QObject *parent)
    : TimerClass(5s, parent)
    , nsLookup(nsLookup)
    , javascriptWrapper(javascriptWrapper)
    , db(db)
{
    Q_CONNECT(this, &Transactions::showNotification, &mainWin, &MainWindow::showNotification);
    Q_CONNECT(&authManager, &auth::Auth::logined, this, &Transactions::onLogined);
    Q_CONNECT(this, &Transactions::registerAddresses, this, &Transactions::onRegisterAddresses);
    Q_CONNECT(this, &Transactions::getAddresses, this, &Transactions::onGetAddresses);
    Q_CONNECT(this, &Transactions::setCurrentGroup, this, &Transactions::onSetCurrentGroup);
    Q_CONNECT(this, &Transactions::getTxs, this, &Transactions::onGetTxs);
    Q_CONNECT(this, &Transactions::getTxs2, this, &Transactions::onGetTxs2);
    Q_CONNECT(this, &Transactions::getTxsAll, this, &Transactions::onGetTxsAll);
    Q_CONNECT(this, &Transactions::getTxsAll2, this, &Transactions::onGetTxsAll2);
    Q_CONNECT(this, &Transactions::getForgingTxs, this, &Transactions::onGetForgingTxs);
    Q_CONNECT(this, &Transactions::getDelegateTxs, this, &Transactions::onGetDelegateTxs);
    Q_CONNECT(this, &Transactions::getDelegateTxs2, this, &Transactions::onGetDelegateTxs2);
    Q_CONNECT(this, &Transactions::getLastForgingTx, this, &Transactions::onGetLastForgingTx);
    Q_CONNECT(this, &Transactions::calcBalance, this, &Transactions::onCalcBalance);
    Q_CONNECT(this, &Transactions::sendTransaction, this, &Transactions::onSendTransaction);
    Q_CONNECT(this, &Transactions::getTxFromServer, this, &Transactions::onGetTxFromServer);
    Q_CONNECT(this, &Transactions::getLastUpdateBalance, this, &Transactions::onGetLastUpdateBalance);
    Q_CONNECT(this, &Transactions::getNonce, this, &Transactions::onGetNonce);
    Q_CONNECT(this, &Transactions::clearDb, this, &Transactions::onClearDb);

    Q_REG(RegisterAddressCallback, "RegisterAddressCallback");
    Q_REG(GetTxsCallback, "GetTxsCallback");
    Q_REG(CalcBalanceCallback, "CalcBalanceCallback");
    Q_REG(SetCurrentGroupCallback, "SetCurrentGroupCallback");
    Q_REG(GetAddressesCallback, "GetAddressesCallback");
    Q_REG(GetTxCallback, "GetTxCallback");
    Q_REG(GetLastUpdateCallback, "GetLastUpdateCallback");
    Q_REG(GetNonceCallback, "GetNonceCallback");
    Q_REG(SendTransactionCallback, "SendTransactionCallback");
    Q_REG(ClearDbCallback, "ClearDbCallback");

    Q_REG2(size_t, "size_t", false);
    Q_REG2(seconds, "seconds", false);
    Q_REG(DelegateStatus, "DelegateStatus");
    Q_REG(SendParameters, "SendParameters");

    Q_REG(std::vector<AddressInfo>, "std::vector<AddressInfo>");

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("timeouts_sec/transactions"), "settings timeout not found");
    timeout = seconds(settings.value("timeouts_sec/transactions").toInt());

    client.setParent(this);
    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &Transactions::callbackCall);
    client.moveToThread(TimerClass::getThread());

    Q_CONNECT(&tcpClient, &HttpSimpleClient::callbackCall, this, &Transactions::callbackCall);
    tcpClient.moveToThread(TimerClass::getThread());

    timerSendTx.moveToThread(TimerClass::getThread());
    timerSendTx.setInterval(milliseconds(100).count());
    Q_CONNECT(&timerSendTx, &QTimer::timeout, this, &Transactions::onFindTxOnTorrentEvent);

    javascriptWrapper.setTransactions(*this);

    moveToThread(TimerClass::getThread()); // TODO вызывать в TimerClass
}

Transactions::~Transactions() {
    TimerClass::exit();
}

void Transactions::startMethod() {
    // empty
}

void Transactions::finishMethod() {
    emit timerSendTx.stop();
}

uint64_t Transactions::calcCountTxs(const QString &address, const QString &currency) const {
    return static_cast<uint64_t>(db.getPaymentsCountForAddress(address, currency));
}

void Transactions::newBalance(const QString &address, const QString &currency, uint64_t savedCountTxs, uint64_t confirmedCountTxsInThisLoop, const BalanceInfo &balance, const BalanceInfo &curBalance, const std::vector<Transaction> &txs, const std::shared_ptr<ServersStruct> &servStruct) {
    const uint64_t currCountTxs = calcCountTxs(address, currency);
    CHECK(savedCountTxs == currCountTxs, "Trancastions in db on address " + address.toStdString() + " " + currency.toStdString() + " changed");
    auto transactionGuard = db.beginTransaction();
    for (const Transaction &tx: txs) {
        db.addPayment(tx);
    }
    db.setBalance(currency, address, balance);
    transactionGuard.commit();

    BalanceInfo balanceCopy = balance;
    balanceCopy.savedTxs = std::min(confirmedCountTxsInThisLoop, balance.countTxs);
    if (balanceCopy.savedTxs == balance.countTxs) {
        BigNumber sumch = (balance.received - balance.spent) - (curBalance.received - curBalance.spent);
        QString value = sumch.getFracDecimal(BNModule);
        // remove '-' if present
        if (value.front() == QChar('-'))
            value = value.right(value.size() - 1);
        if (sumch.isNegative())
            value.prepend(tr("Out") + QStringLiteral(" "));
        else if (!sumch.isZero())
            value.prepend(tr("In") + QStringLiteral(" "));
        value += QStringLiteral(" ") + currency.toUpper();
        emit showNotification(tr("Balance for address %1 changed").arg(address), value);
    }
    emit javascriptWrapper.newBalanceSig(address, currency, balanceCopy);
    updateBalanceTime(currency, servStruct);
}

void Transactions::updateBalanceTime(const QString &currency, const std::shared_ptr<ServersStruct> &servStruct) {
    if (servStruct == nullptr) {
        return;
    }
    CHECK(servStruct->currency == currency, "Incorrect servStruct currency");
    servStruct->countRequests--;
    if (servStruct->countRequests == 0) {
        const system_time_point now = ::system_now();
        lastSuccessUpdateTimestamps[currency] = now;
    }
}

void Transactions::processPendings() {
    const auto processPendingTx = [this](const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const Transaction tx = parseGetTxResponse(QString::fromStdString(response), "", "");
        if (tx.status != Transaction::PENDING) {
            const auto foundIter = std::remove_if(pendingTxsAfterSend.begin(), pendingTxsAfterSend.end(), [txHash=tx.tx](const auto &pair){
                return pair.first == txHash;
            });
            const bool found = foundIter != pendingTxsAfterSend.end();
            pendingTxsAfterSend.erase(foundIter, pendingTxsAfterSend.end());
            if (found) {
                emit javascriptWrapper.transactionStatusChanged2Sig(tx.tx, tx);
            }
        }
    };

    LOG << PeriodicLog::make("pas") << "Pending after send: " << pendingTxsAfterSend.size();

    const auto copyPending = pendingTxsAfterSend;
    for (const auto &pair: copyPending) {
        const QString message = makeGetTxRequest(pair.first);
        for (const QString &server: pair.second) {
            client.sendMessagePost(server, message, processPendingTx, timeout);
        }
    }
}

void Transactions::processAddressMth(const std::vector<std::pair<QString, std::vector<QString>>> &addressesAndUnconfirmedTxs, const QString &currency, const std::vector<QString> &servers, const std::shared_ptr<ServersStruct> &servStruct) {
    if (servers.empty()) {
        return;
    }
    if (addressesAndUnconfirmedTxs.empty()) {
        return;
    }
    const auto processPendingTx = [this, currency](const QString &address, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const Transaction tx = parseGetTxResponse(QString::fromStdString(response), address, currency);
        if (tx.status != Transaction::PENDING) {
            db.updatePayment(address, currency, tx.tx, tx.blockNumber, tx.blockIndex, tx);
            emit javascriptWrapper.transactionStatusChangedSig(address, currency, tx.tx, tx);
            emit javascriptWrapper.transactionStatusChanged2Sig(tx.tx, tx);
        }
    };

    const auto getBlockHeaderCallback = [this, currency, servStruct](const QString &address, const BalanceInfo &balance, uint64_t savedCountTxs, uint64_t confirmedCountTxsInThisLoop, std::vector<Transaction> txs, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const BlockInfo bi = parseGetBlockInfoResponse(QString::fromStdString(response));
        for (Transaction &tx: txs) {
            if (tx.blockNumber == bi.number) {
                tx.blockHash = bi.hash;
            }
        }
        const BalanceInfo confirmedBalance = db.getBalance(currency, address);
        newBalance(address, currency, savedCountTxs, confirmedCountTxsInThisLoop, balance, confirmedBalance, txs, servStruct);
    };

    const auto processNewTransactions = [this, currency, getBlockHeaderCallback](const QString &address, const BalanceInfo &balance, uint64_t savedCountTxs, uint64_t confirmedCountTxsInThisLoop, const std::vector<Transaction> &txs, const QUrl &server) {
        const auto maxElement = std::max_element(txs.begin(), txs.end(), [](const Transaction &first, const Transaction &second) {
            return first.blockNumber < second.blockNumber;
        });
        CHECK(maxElement != txs.end(), "Incorrect max element");
        const int64_t blockNumber = maxElement->blockNumber;
        const QString request = makeGetBlockInfoRequest(blockNumber);
        client.sendMessagePost(server, request, std::bind(getBlockHeaderCallback, address, balance, savedCountTxs, confirmedCountTxsInThisLoop, txs, _1, _2), timeout);
    };

    const auto getBalanceConfirmeCallback = [currency, processNewTransactions](const QString &address, const BalanceInfo &serverBalance, uint64_t savedCountTxs, uint64_t confirmedCountTxsInThisLoop, const std::vector<Transaction> &txs, const QUrl &server, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const BalanceInfo balance = parseBalanceResponse(QString::fromStdString(response));
        const uint64_t countInServer = balance.countTxs;
        const uint64_t countSave = serverBalance.countTxs;
        if (countInServer - countSave <= ADD_TO_COUNT_TXS) {
            LOG << "Balance " << address << " confirmed";
            processNewTransactions(address, serverBalance, savedCountTxs, confirmedCountTxsInThisLoop, txs, server);
        }
    };

    const auto getHistoryCallback = [this, currency, getBalanceConfirmeCallback](const QString &address, const BalanceInfo &serverBalance, uint64_t savedCountTxs, uint64_t confirmedCountTxsInThisLoop, const QUrl &server, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const std::vector<Transaction> txs = parseHistoryResponse(address, currency, QString::fromStdString(response));

        LOG << "geted with duplicates " << address << " " << txs.size();

        const QString requestBalance = makeGetBalanceRequest(address);

        client.sendMessagePost(server, requestBalance, std::bind(getBalanceConfirmeCallback, address, serverBalance, savedCountTxs, confirmedCountTxsInThisLoop, txs, server, _1, _2), timeout);
    };

    const auto getBalanceCallback = [this, servStruct, addressesAndUnconfirmedTxs, currency, getHistoryCallback, processPendingTx](const std::vector<QUrl> &servers, const std::vector<std::tuple<std::string, SimpleClient::ServerException>> &responses) {
        CHECK(!servers.empty(), "Incorrect response size");
        CHECK(servers.size() == responses.size(), "Incorrect response size");

        std::vector<std::pair<QUrl, BalanceInfo>> bestAnswers(addressesAndUnconfirmedTxs.size());
        for (size_t i = 0; i < responses.size(); i++) {
            const auto &responsePair = responses[i];
            const auto &exception = std::get<SimpleClient::ServerException>(responsePair);
            const std::string &response = std::get<std::string>(responsePair);
            const QUrl &server = servers[i];
            if (!exception.isSet()) {
                const std::vector<BalanceInfo> balancesResponse = parseBalancesResponse(QString::fromStdString(response));
                CHECK(balancesResponse.size() == addressesAndUnconfirmedTxs.size(), "Incorrect balances response");
                for (size_t j = 0; j < balancesResponse.size(); j++) {
                    const BalanceInfo &balanceResponse = balancesResponse[j];
                    const QString &address = addressesAndUnconfirmedTxs[j].first;
                    CHECK(balanceResponse.address == address, "Incorrect response: address not equal. Expected " + address.toStdString() + ". Received " + balanceResponse.address.toStdString());
                    if (balanceResponse.currBlockNum > bestAnswers[j].second.currBlockNum) {
                        bestAnswers[j].second = balanceResponse;
                        bestAnswers[j].first = server;
                    }
                }
            } else {
                if (exception.isTimeout()) {
                    emit nsLookup.rejectServer(server.toString());
                }
            }
        }

        CHECK(bestAnswers.size() == addressesAndUnconfirmedTxs.size(), "Ups");
        for (size_t i = 0; i < bestAnswers.size(); i++) {
            const QUrl &bestServer = bestAnswers[i].first;
            const QString &address = addressesAndUnconfirmedTxs[i].first;
            const BalanceInfo &serverBalance = bestAnswers[i].second;
            const std::vector<QString> &pendingTxs = addressesAndUnconfirmedTxs[i].second;

            if (bestServer.isEmpty()) {
                LOG << PeriodicLog::makeAuto("t_err") << "Best server with txs not found. Error: " << std::get<SimpleClient::ServerException>(responses[0]).toString();
                return;
            }

            const uint64_t countAll = calcCountTxs(address, currency);
            const uint64_t countInServer = serverBalance.countTxs;
            LOG << PeriodicLog::make(std::string("t_") + currency[0].toLatin1() + "," + address.right(4).toStdString()) << "Automatic get txs " << address << " " << currency << " " << countAll << " " << countInServer;
            if (countAll < countInServer) {
                processCheckTxsOneServer(address, currency, bestServer);

                const uint64_t countMissingTxs = countInServer - countAll;
                const uint64_t beginTx = countMissingTxs >= MAX_TXS_IN_RESPONSE ? countMissingTxs - MAX_TXS_IN_RESPONSE : 0;
                const uint64_t requestCountTxs = std::min(countMissingTxs, MAX_TXS_IN_RESPONSE) + ADD_TO_COUNT_TXS;
                const QString requestForTxs = makeGetHistoryRequest(address, true, beginTx, requestCountTxs);

                client.sendMessagePost(bestServer, requestForTxs, std::bind(getHistoryCallback, address, serverBalance, countAll, requestCountTxs + countAll, bestServer, _1, _2), timeout);
            } else if (countAll > countInServer) {
                //removeAddress(address, currency); Не удаляем, так как при перевыкачки базы начнется свистопляска
            } else {
                const BalanceInfo confirmedBalance = db.getBalance(currency, address);
                if (confirmedBalance.countTxs != countInServer) {
                    newBalance(address, currency, countAll, serverBalance.countTxs, serverBalance, confirmedBalance, {}, servStruct);
                } else {
                    updateBalanceTime(currency, servStruct);
                }
            }

            if (!pendingTxs.empty()) {
                LOG << PeriodicLog::make("pt_" + address.right(4).toStdString()) << "Pending txs: " << pendingTxs.size();
            }

            for (const QString &txHash: pendingTxs) {
                const QString message = makeGetTxRequest(txHash);
                client.sendMessagePost(bestServer, message, std::bind(processPendingTx, address, _1, _2), timeout);
            }
        }
    };

    std::vector<QString> addresses;
    addresses.reserve(addressesAndUnconfirmedTxs.size());
    std::transform(addressesAndUnconfirmedTxs.begin(), addressesAndUnconfirmedTxs.end(), std::back_inserter(addresses), [](const auto &pair) {
        return pair.first;
    });
    const QString requestBalance = makeGetBalancesRequest(addresses);
    const std::vector<QUrl> urls(servers.begin(), servers.end());
    client.sendMessagesPost(addresses[0].toStdString(), urls, requestBalance, std::bind(getBalanceCallback, urls, _1), timeout);
}

std::vector<AddressInfo> Transactions::getAddressesInfos(const QString &group) {
    return db.getTrackedForGroup(group);
}

BalanceInfo Transactions::getBalance(const QString &address, const QString &currency) {
    BalanceInfo balance = db.getBalance(currency, address);

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

void Transactions::removeAddress(const QString &address, const QString &currency) {
    LOG << "Remove txs " << address << " " << currency;
    db.removePaymentsForDest(address, currency);
    db.removeBalance(currency, address);
}

void Transactions::processCheckTxsInternal(const QString &address, const QString &currency, const QUrl &server, const Transaction &tx, int64_t serverBlockNumber) {
    const auto getBlockInfoCallback = [address, currency, this] (const QString &hash, const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const BlockInfo bi = parseGetBlockInfoResponse(QString::fromStdString(response));
        if (bi.hash != hash) {
            removeAddress(address, currency);
        }
    };

    if (tx.blockNumber > serverBlockNumber) {
        removeAddress(address, currency);
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
        if (maxServer.isEmpty()) {
            LOG << PeriodicLog::makeAuto("t_err2") << "Best server with txs not found. Error: " << std::get<SimpleClient::ServerException>(responses[0]).toString();
        } else {
            processCheckTxsInternal(address, currency, maxServer, lastTx, maxBlockNumber);
        }
    };

    const QString countBlocksRequest = makeGetCountBlocksRequest();
    const std::vector<QUrl> urls(servers.begin(), servers.end());
    client.sendMessagesPost(address.toStdString(), urls, countBlocksRequest, std::bind(countBlocksCallback, urls, _1), timeout);
}

void Transactions::timerMethod() {
    static const size_t COUNT_PARALLEL_REQUESTS = 15;
    static const size_t MAXIMUM_ADDRESSES_IN_BATCH = 20;

    const auto CHECK_TXS_PERIOD = 3min;


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
    QString currentType;
    QString currentCurrency;
    std::map<QString, std::shared_ptr<ServersStruct>> servStructs;
    std::vector<std::pair<QString, std::vector<QString>>> batch;
    size_t countParallelRequests = 0;

    const auto processBatch = [this, &batch, &countParallelRequests, &servStructs](const QString &currentCurrency, const QString &currentType) {
        if (servStructs.find(currentCurrency) != servStructs.end()) {
            nsLookup.getRandomServers(currentType, 3, 3, NsLookup::GetServersCallback([this, batch, currentCurrency, currentType, servStruct=servStructs.at(currentCurrency)](const std::vector<QString> &servers) {
                if (servers.empty()) {
                    LOG << PeriodicLog::makeAuto("t_s0") << "Warn: servers empty: " << currentType;
                    return;
                }
                processAddressMth(batch, currentCurrency, servers, servStruct);
            }, [](const TypedException &error) {
                LOG << "Error while get servers: " << error.description;
            }, signalFunc));

            batch.clear();
            countParallelRequests++;
        } else {
            // В предыдущий раз список серверов оказался пустым, поэтому структуру мы не заполнили. Пропускаем
        }
    };

    const time_point now = ::now();
    while (posInAddressInfos < addressesInfos.size()) {
        const AddressInfo &addr = addressesInfos[posInAddressInfos];
        if ((!currentCurrency.isEmpty() && (addr.currency != currentCurrency || addr.type != currentType)) || batch.size() >= MAXIMUM_ADDRESSES_IN_BATCH) {
            processBatch(currentCurrency, currentType);
            if (countParallelRequests >= COUNT_PARALLEL_REQUESTS) {
                break;
            }
            currentCurrency = addr.currency;
            currentType = addr.type;
        }
        if (currentCurrency.isEmpty()) {
            currentCurrency = addr.currency;
        }
        if (currentType.isEmpty()) {
            currentType = addr.type;
        }

        const auto found = servStructs.find(addr.currency);
        if (found == servStructs.end()) {
            servStructs.emplace(std::piecewise_construct, std::forward_as_tuple(addr.currency), std::forward_as_tuple(std::make_shared<ServersStruct>(addr.currency)));
        }
        servStructs.at(addr.currency)->countRequests++; // Не очень хорошо здесь прибавлять по 1, но пофиг
        if (now - lastCheckTxsTime >= CHECK_TXS_PERIOD) {
            nsLookup.getRandomServers(addr.type, 3, 3, NsLookup::GetServersCallback([this, address=addr.address, currency=addr.currency](const std::vector<QString> &servers) {
                processCheckTxs(address, currency, servers);
            }, [](const TypedException &error) {
                LOG << "Error while get servers: " << error.description;
            }, signalFunc));
        }
        const std::vector<Transaction> pendingTxs = db.getPaymentsForAddressPending(addr.address, addr.currency, true);
        std::vector<QString> pendingTxsStrs;
        pendingTxsStrs.reserve(pendingTxs.size());
        std::transform(pendingTxs.begin(), pendingTxs.end(), std::back_inserter(pendingTxsStrs), std::mem_fn(&Transaction::tx));

        batch.emplace_back(addr.address, pendingTxsStrs);

        posInAddressInfos++;
    }
    processBatch(currentCurrency, currentType);

    if (now - lastCheckTxsTime >= CHECK_TXS_PERIOD && posInAddressInfos >= addressesInfos.size()) {
        LOG << "All txs checked";
        lastCheckTxsTime = now;
    }

    processPendings();
}

void Transactions::fetchBalanceAddress(const QString &address) {
    const std::vector<AddressInfo> infos = getAddressesInfos(currentGroup);
    std::vector<AddressInfo> addressInfos;
    std::copy_if(infos.begin(), infos.end(), std::back_inserter(addressInfos), [&address](const AddressInfo &info) {
        return info.address == address;
    });

    LOG << "Found " << addressInfos.size() << " records on adrress " << address;
    for (const AddressInfo &addr: addressInfos) {
        nsLookup.getRandomServers(addr.type, 3, 3, NsLookup::GetServersCallback([this, addr](const std::vector<QString> &servers) {
            if (servers.empty()) {
                LOG << "Warn: servers empty: " << addr.type;
                return;
            }

            std::vector<std::pair<QString, std::vector<QString>>> batch;
            batch.emplace_back(addr.address, std::vector<QString>());
            processAddressMth(batch, addr.currency, servers, nullptr);
        }, [](const TypedException &e) {
            LOG << "Error while get servers: " << e.description;
        }, signalFunc));
    }
}

void Transactions::onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        auto transactionGuard = db.beginTransaction();
        for (const AddressInfo &address: addresses) {
            db.addTracked(address);
        }
        transactionGuard.commit();
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetAddresses(const QString &group, const GetAddressesCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        std::vector<AddressInfo> result = getAddressesInfos(group);
        for (AddressInfo &info: result) {
            info.balance = getBalance(info.address, info.currency);
            info.balance.savedTxs = info.balance.countTxs;
        }
        return result;
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onSetCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        //currentGroup = group;
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetTxs(const QString &address, const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback)
{
    Q_UNUSED(address);
    Q_UNUSED(currency);
    Q_UNUSED(fromTx);
    Q_UNUSED(count);
    Q_UNUSED(asc);
BEGIN_SLOT_WRAPPER
    // TODO
    runAndEmitCallback([&, this] {
        return std::vector<Transaction>();
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetTxs2(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return db.getPaymentsForAddress(address, convertCurrency(currency), from, count, asc);
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetTxsAll(const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback)
{
    Q_UNUSED(currency);
    Q_UNUSED(fromTx);
    Q_UNUSED(count);
    Q_UNUSED(asc);
BEGIN_SLOT_WRAPPER
    // TODO
    runAndEmitCallback([&, this] {
        return std::vector<Transaction>();
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetTxsAll2(const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return db.getPaymentsForCurrency(convertCurrency(currency), from, count, asc);
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetForgingTxs(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return db.getForgingPaymentsForAddress(address, convertCurrency(currency), from, count, asc);
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetDelegateTxs(const QString &address, const QString &currency, const QString &to, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return db.getDelegatePaymentsForAddress(address, to, convertCurrency(currency), from, count, asc);
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetDelegateTxs2(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return db.getDelegatePaymentsForAddress(address, convertCurrency(currency), from, count, asc);
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetLastForgingTx(const QString &address, const QString &currency, const GetTxCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return db.getLastForgingTransaction(address, convertCurrency(currency));
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onCalcBalance(const QString &address, const QString &currency, const CalcBalanceCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        BalanceInfo balance = getBalance(address, convertCurrency(currency));
        balance.savedTxs = balance.countTxs;
        return balance;
    }, callback);
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
            client.sendMessagePost(server, message, [this, server, hash, isFirst, requestId=watcher.requestId, serversCopy](const std::string &response, const SimpleClient::ServerException &exception) {
                auto found = sendTxWathcers.find(hash);
                if (found == sendTxWathcers.end()) {
                    return;
                }
                if (!exception.isSet()) {
                    try {
                        const Transaction tx = parseGetTxResponse(QString::fromStdString(response), "", "");
                        emit javascriptWrapper.transactionInTorrentSig(requestId, server, QString::fromStdString(hash), tx, TypedException());
                        if (tx.status == Transaction::Status::PENDING) {
                            pendingTxsAfterSend.emplace_back(tx.tx, serversCopy);
                        }
                        if (*isFirst) {
                            *isFirst = false;
                            fetchBalanceAddress(tx.from);
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

void Transactions::addTrackedForCurrentLogin()
{
    if (!mainJavascriptWrapper) {
        return;
    }
    const auto errorCallback = [](const TypedException &e) {
        LOG << "Error: " << e.description;
    };

    const auto processWallets = [this](const QString &currency, const QString &type, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) {
        const QString uName = userName.isEmpty() ? "empty" : userName;
        if (uName != currentGroup) {
            // TODO придумать, как перезапросить список кошельков
            return;
        }
        auto transactionGuard = db.beginTransaction();
        db.removeTrackedForGroup(currency, currentGroup);
        for (const Wallet::WalletInfo &wallet: walletAddresses) {
            db.addTracked(currency, wallet.address, "", type, currentGroup);
        }
        transactionGuard.commit();
    };

    emit mainJavascriptWrapper->getListWallets(JavascriptWrapper::WalletCurrency::Mth, JavascriptWrapper::WalletsListCallback([processWallets](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) {
        Q_UNUSED(hwid);
        processWallets("mhc", TorrentTypeMainNet, userName, walletAddresses);
    }, errorCallback, signalFunc));
    emit mainJavascriptWrapper->getListWallets(JavascriptWrapper::WalletCurrency::Tmh, JavascriptWrapper::WalletsListCallback([processWallets](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) {
        Q_UNUSED(hwid);
        processWallets("tmh", TorrentTypeDevNet, userName, walletAddresses);
    }, errorCallback, signalFunc));
}

void Transactions::onLogined(bool isInit, const QString login) {
BEGIN_SLOT_WRAPPER
    if (!isInit) {
        return;
    }
    QString newGroup;
    if (login.isEmpty()) {
        newGroup = "_unregistered";
    } else {
        newGroup = login;
    }
    /*if (newGroup == currentGroup) {
        return;
    }*/
    currentGroup = newGroup;
    addTrackedForCurrentLogin();
END_SLOT_WRAPPER
}

void Transactions::onMthWalletCreated(const QString &name)
{
    Q_UNUSED(name);
BEGIN_SLOT_WRAPPER
    addTrackedForCurrentLogin();
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
    runAndEmitCallback([&, this] {
        const QString request = makeSendTransactionRequest(to, value, nonce, data, fee, pubkey, sign);
        nsLookup.getRandomServers(sendParams.typeSend, sendParams.countServersSend, sendParams.countServersSend, NsLookup::GetServersCallback([this, sendParams, requestId, request, callback](const std::vector<QString> &servers) {
            const size_t countServersSend = sendParams.countServersSend;
            CHECK_TYPED(!servers.empty(), TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "Not enough servers send");
            const size_t remainServersSend = countServersSend - servers.size();
            for (size_t i = 0; i < remainServersSend; i++) {
                emit javascriptWrapper.sendedTransactionsResponseSig(requestId, "", "", TypedException(TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "dns return less laid"));
            }

            nsLookup.getRandomServers(sendParams.typeGet, sendParams.countServersGet, sendParams.countServersGet, NsLookup::GetServersCallback([this, servers, request, requestId, sendParams](const std::vector<QString> &serversGet) {
                CHECK_TYPED(!serversGet.empty(), TypeErrors::TRANSACTIONS_SERVER_NOT_FOUND, "Not enough servers get");

                for (const QString &server: servers) {
                    tcpClient.sendMessagePost(server, request, [this, server, requestId, sendParams, serversGet](const std::string &response, const TypedException &error) {
                        QString result;
                        const TypedException exception = apiVrapper2([&] {
                            if (error.isSet()) {
                                nsLookup.rejectServer(server);
                            }
                            CHECK_TYPED(!error.isSet(), TypeErrors::TRANSACTIONS_SERVER_SEND_ERROR, error.description + ". " + server.toStdString());
                            result = parseSendTransactionResponse(QString::fromStdString(response));
                            addToSendTxWatcher(requestId, result.toStdString(), sendParams.countServersGet, serversGet, sendParams.timeout);
                        });
                        emit javascriptWrapper.sendedTransactionsResponseSig(requestId, server, result, exception);
                    }, timeout);
                }
            }, callback, signalFunc)); // Получаем список серверов здесь, чтобы здесь же обработать ошибки
        }, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetNonce(const QString &requestId, const QString &from, const SendParameters &sendParams, const GetNonceCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&, this]{
        nsLookup.getRandomServers(sendParams.typeGet, sendParams.countServersGet, sendParams.countServersGet, NsLookup::GetServersCallback([this, requestId, from, callback](const std::vector<QString> &servers) {
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
        }, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&, this] {
        const QString message = makeGetTxRequest(txHash);

        nsLookup.getRandomServers(type, 1, 1, NsLookup::GetServersCallback([this, message, callback](const std::vector<QString> &servers){
            CHECK(!servers.empty(), "Not enough servers");
            const QString &server = servers[0];

            client.sendMessagePost(server, message, [this, callback](const std::string &response, const SimpleClient::ServerException &error) mutable {
                Transaction tx;
                const TypedException exception = apiVrapper2([&] {
                    CHECK_TYPED(!error.isSet(), TypeErrors::CLIENT_ERROR, error.description);
                    tx = parseGetTxResponse(QString::fromStdString(response), "", "");
                });
                callback.emitFunc(exception, tx);
            }, timeout);
        }, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onGetLastUpdateBalance(const QString &currency, const GetLastUpdateCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        const system_time_point now = ::system_now();
        auto found = lastSuccessUpdateTimestamps.find(currency);
        system_time_point result;
        if (found != lastSuccessUpdateTimestamps.end()) {
            result = found->second;
        }
        return std::make_tuple(result, now);
    }, callback);
END_SLOT_WRAPPER
}

void Transactions::onClearDb(const QString &currency, const ClearDbCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        db.removePaymentsForCurrency(currency);
        nsLookup.resetFile();
    }, callback);
END_SLOT_WRAPPER
}

SendParameters parseSendParams(const QString &paramsJson) {
    return parseSendParamsInternal(paramsJson);
}

}
