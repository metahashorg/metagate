#include "Transactions.h"

#include "check.h"
#include "SlotWrapper.h"

#include "NsLookup.h"

namespace transactions {

Transactions::Transactions(NsLookup &nsLookup, QObject *parent)
    : TimerClass(1s, parent)
    , nsLookup(nsLookup)
{
    CHECK(connect(this, &Transactions::timerEvent, this, &Transactions::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &Transactions::startedEvent, this, &Transactions::onRun), "not connect run");

    CHECK(connect(this, &Transactions::registerAddress, this, &Transactions::onRegisterAddress), "not connect onRegisterAddress");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Transactions::onCallbackCall), "not connect");
    client.moveToThread(&thread1);
    moveToThread(&thread1);
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
