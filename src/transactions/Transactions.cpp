#include "Transactions.h"

#include "check.h"
#include "SlotWrapper.h"

#include "NsLookup.h"

#include "TransactionsMessages.h"

#include <memory>

namespace transactions {

static const uint64_t ADD_TO_COUNT_TXS = 10;

Transactions::Transactions(NsLookup &nsLookup, QObject *parent)
    : TimerClass(2s, parent)
    , nsLookup(nsLookup)
{
    CHECK(connect(this, &Transactions::timerEvent, this, &Transactions::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &Transactions::startedEvent, this, &Transactions::onRun), "not connect run");

    CHECK(connect(this, &Transactions::registerAddress, this, &Transactions::onRegisterAddress), "not connect onRegisterAddress");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Transactions::onCallbackCall), "not connect");
    client.moveToThread(&thread1);
    moveToThread(&thread1); // TODO вызывать в TimerClass
}

void Transactions::onCallbackCall(std::function<void()> callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void Transactions::onRun() {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
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

void Transactions::processAddressMth(const QString &address, const std::vector<QString> &servers) {
    if (servers.empty()) {
        return;
    }

    std::shared_ptr<BalanceStruct> balanceStruct = std::make_shared<BalanceStruct>(address);
    balanceStruct->countResponses = servers.size();
    for (const QString &server: servers) {
        const QString requestBalance = makeGetBalanceRequest(address);
        const auto getBalanceCallback = [this, balanceStruct, server](const std::string &response) {
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
                if (countAll < countInServer) {
                    const uint64_t countMissingTxs = countInServer - countAll;
                    const uint64_t requestCountTxs = countMissingTxs + ADD_TO_COUNT_TXS;
                    const QString requestForTxs = makeGetHistoryRequest(balanceStruct->address, true, requestCountTxs);

                    const auto getHistoryCallback = [this, balanceStruct, server](const std::string &response) {
                        CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Incorrect response");
                        const std::vector<TxResponse> txs = parseHistoryResponse(balanceStruct->address, QString::fromStdString(response));

                        const QString requestBalance = makeGetBalanceRequest(balanceStruct->address);
                        const auto getBalance2Callback = [this, balanceStruct, server](const std::string &response) {
                            CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Incorrect response");
                            const BalanceResponse balance = parseBalanceResponse(QString::fromStdString(response));
                            const uint64_t countInServer = balance.countReceived + balance.countSpent;
                            const uint64_t countSave = balanceStruct->balance.countReceived + balanceStruct->balance.countSpent;
                            if (countInServer - countSave <= ADD_TO_COUNT_TXS) {
                                // Сохраняем транзакции в bd
                                // emit сигнал с сохраненным балансом
                                getFullTxs[balanceStruct->address] = false;
                            } else {
                                getFullTxs[balanceStruct->address] = true;
                            }
                        };
                        client.sendMessagePost(server, requestBalance, getBalance2Callback, 1s);
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
    // Получить список отслеживаемых
    // Отсортировать по type
    // Завести массив под сервера (и сохраненный type)
    // Идти по списку отслеживаемых, если type поменялся, то перезагрузить массив серверов
    // Фигачить запрос
END_SLOT_WRAPPER
}

void Transactions::onRegisterAddress(const QString &currency, const QString &address, const QString &type, const RegisterAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    // Положить в бд
    // emit callback
END_SLOT_WRAPPER
}

}
