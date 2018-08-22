#include "Transactions.h"

#include "check.h"
#include "SlotWrapper.h"

#include "NsLookup.h"

#include "TransactionsMessages.h"
#include "TransactionsJavascript.h"

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
    CHECK(connect(this, &Transactions::setCurrentGroup, this, &Transactions::onSetCurrentGroup), "not connect onSetCurrentGroup");
    CHECK(connect(this, &Transactions::getTxs, this, &Transactions::onGetTxs), "not connect onGetTxs");
    CHECK(connect(this, &Transactions::getTxs2, this, &Transactions::onGetTxs2), "not connect onGetTxs2");
    CHECK(connect(this, &Transactions::getTxsAll, this, &Transactions::onGetTxsAll), "not connect onGetTxsAll");
    CHECK(connect(this, &Transactions::getTxsAll2, this, &Transactions::onGetTxsAll2), "not connect onGetTxsAll2");
    CHECK(connect(this, &Transactions::calcBalance, this, &Transactions::onCalcBalance), "not connect onCalcBalance");

    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<RegisterAddressCallback>("RegisterAddressCallback");
    qRegisterMetaType<GetTxsCallback>("GetTxsCallback");
    qRegisterMetaType<CalcBalanceCallback>("CalcBalanceCallback");
    qRegisterMetaType<SetCurrentGroupCallback>("SetCurrentGroupCallback");
    qRegisterMetaType<SetCurrentGroupCallback>("SetCurrentGroupCallback");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Transactions::onCallbackCall), "not connect");
    client.moveToThread(&thread1);

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
    BalanceResponse balance;

    QString server;

    BalanceStruct(const QString &address)
        : address(address)
    {}
};

void Transactions::newBalance(const QString &address, const QString &currency, const BalanceResponse &balance, const std::vector<Transaction> &txs) {
    // Сохраняем транзакции в bd
    emit javascriptWrapper.newBalanceSig(address, currency, balance);
    getFullTxs[std::make_pair(currency, address)] = false;
}

void Transactions::processAddressMth(const QString &address, const QString &currency, const std::vector<QString> &servers) {
    if (servers.empty()) {
        return;
    }

    std::shared_ptr<BalanceStruct> balanceStruct = std::make_shared<BalanceStruct>(address);
    balanceStruct->countResponses = servers.size();
    for (const QString &server: servers) {
        const QString requestBalance = makeGetBalanceRequest(address);
        const auto getBalanceCallback = [this, balanceStruct, server, currency](const std::string &response) {
            balanceStruct->countResponses--;

            if (response != SimpleClient::ERROR_BAD_REQUEST) {
                const BalanceResponse balanceResponse = parseBalanceResponse(QString::fromStdString(response));
                CHECK(balanceResponse.address == balanceStruct->address, "Incorrect response: address not equal. Expected " + balanceStruct->address.toStdString() + ". Received " + balanceResponse.address.toStdString());
                if (balanceResponse.currBlockNum > balanceStruct->balance.currBlockNum) {
                    balanceStruct->balance = balanceResponse;
                    balanceStruct->server = server;
                }
            }

            if (balanceStruct->countResponses == 0 && !balanceStruct->server.isEmpty()) {
                // Получаем количество received и количество spent
                const uint64_t countReceived = 0;
                const uint64_t countSpent = 0;
                const uint64_t countAll = countReceived + countSpent;
                const uint64_t countInServer = balanceStruct->balance.countReceived + balanceStruct->balance.countSpent;
                LOG << "Automatic get txs " << balanceStruct->address << " " << countAll << " " << countInServer;
                if (countAll < countInServer) {
                    const uint64_t countMissingTxs = countInServer - countAll;
                    const uint64_t requestCountTxs = countMissingTxs + ADD_TO_COUNT_TXS;
                    const bool isToTxs = !getFullTxs[std::make_pair(currency, balanceStruct->address)];
                    const QString requestForTxs = makeGetHistoryRequest(balanceStruct->address, isToTxs, requestCountTxs);

                    const auto getHistoryCallback = [this, balanceStruct, server, isToTxs, currency](const std::string &response) {
                        CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Incorrect response");
                        const std::vector<Transaction> txs = parseHistoryResponse(balanceStruct->address, QString::fromStdString(response));

                        LOG << "Txs geted " << balanceStruct->address << " " << txs.size();

                        if (isToTxs) {
                            const QString requestBalance = makeGetBalanceRequest(balanceStruct->address);
                            const auto getBalance2Callback = [this, balanceStruct, server, currency, txs](const std::string &response) {
                                CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Incorrect response");
                                const BalanceResponse balance = parseBalanceResponse(QString::fromStdString(response));
                                const uint64_t countInServer = balance.countReceived + balance.countSpent;
                                const uint64_t countSave = balanceStruct->balance.countReceived + balanceStruct->balance.countSpent;
                                if (countInServer - countSave <= ADD_TO_COUNT_TXS) {
                                    LOG << "Balance " << balanceStruct->address << " confirmed";
                                    newBalance(balanceStruct->address, currency, balanceStruct->balance, txs);
                                } else {
                                    LOG << "Balance " << balanceStruct->address << " not confirmed";
                                    getFullTxs[std::make_pair(currency, balanceStruct->address)] = true;
                                }
                            };
                            client.sendMessagePost(server, requestBalance, getBalance2Callback, 1s);
                        } else {
                            LOG << "Balance " << balanceStruct->address << " confirmed2";
                            newBalance(balanceStruct->address, currency, balanceStruct->balance, txs);
                        }
                    };
                    client.sendMessagePost(server, requestForTxs, getHistoryCallback, 1s);
                }
            }
        };
        client.sendMessagePost(server, requestBalance, getBalanceCallback, 1s);
    }
}

void Transactions::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    // Получить список отслеживаемых по текущей группе
    // Отсортировать по type
    // Завести массив под сервера (и сохраненный type)
    // Идти по списку отслеживаемых, если type поменялся, то перезагрузить массив серверов
    // Фигачить запрос
END_SLOT_WRAPPER
}

void Transactions::onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Положить в бд
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Transactions::onSetCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback) {
BEGIN_SLOT_WRAPPER
    currentGroup = group;
    runCallback(std::bind(callback, TypedException()));
END_SLOT_WRAPPER
}

void Transactions::onGetTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Запросить из bd
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetTxs2(QString address, QString currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Запросить из bd
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Запросить из bd
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onGetTxsAll2(QString currency, int from, int count, bool asc, const GetTxsCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Запросить из bd
    std::vector<Transaction> txs;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, txs, exception));
END_SLOT_WRAPPER
}

void Transactions::onCalcBalance(QString address, QString currency, const CalcBalanceCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Запросить из bd
    BalanceResponse balance;
    const TypedException exception = apiVrapper2([&, this] {

    });
    runCallback(std::bind(callback, balance, exception));
END_SLOT_WRAPPER
}

}
