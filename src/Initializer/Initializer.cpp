#include "Initializer.h"

#include <QSettings>

#include "check.h"
#include "SlotWrapper.h"
#include "Paths.h"

#include "InitializerJavascript.h"
#include "InitInterface.h"

namespace initializer {

InitState::InitState(const QString &type, const QString &subType, const QString &message, const TypedException &exception)
    : type(type)
    , subType(subType)
    , message(message)
    , exception(exception)
{}

Initializer::Initializer(InitializerJavascript &javascriptWrapper, QObject *parent)
    : QObject(parent)
    , javascriptWrapper(javascriptWrapper)
{
    CHECK(connect(this, &Initializer::resendAllStatesSig, this, &Initializer::onResendAllStates), "not connect onGetAllStates");
    CHECK(connect(this, &Initializer::javascriptReadySig, this, &Initializer::onJavascriptReady), "not connect onJavascriptReady");
    CHECK(connect(this, &Initializer::sendState, this, &Initializer::onSendState, Qt::ConnectionType::QueuedConnection), "not connect onSendState");

    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<GetAllStatesCallback>("GetAllStatesCallback");
    qRegisterMetaType<ReadyCallback>("ReadyCallback");
    qRegisterMetaType<InitState>("InitState");
}

Initializer::~Initializer() = default;

void Initializer::complete() {
    isComplete = true;
}

template<typename Func>
void Initializer::runCallback(const Func &callback) {
    emit javascriptWrapper.callbackCall(callback);
}

void Initializer::sendStateToJs(const InitState &state, int number) {
    emit javascriptWrapper.stateChangedSig(number, totalStates, state);
}

void Initializer::sendInitializedToJs() {
    emit javascriptWrapper.initializedSig(TypedException());
}

void Initializer::onSendState(const InitState &state) {
BEGIN_SLOT_WRAPPER
    CHECK(isComplete, "Not complete"); // Запросы приходят в QueuedConnection, поэтому флаг isComplete в этот момент уже должен быть установлен
    CHECK(existStates.size() == states.size(), "Incorrect existStates struct");
    CHECK(existStates.find(std::make_pair(state.type, state.subType)) == existStates.end(), "State already setted");
    CHECK((int)states.size() < totalStates, "Incorrect state number");
    existStates.emplace(state.type, state.subType);
    states.emplace_back(state);
    sendStateToJs(state, (int)states.size() - 1);
    if (!isInitFinished && (int)states.size() == totalStates) {
        isInitFinished = true;
        sendInitializedToJs();
    }
END_SLOT_WRAPPER
}

void Initializer::onResendAllStates(const GetAllStatesCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        if (isComplete) {
            for (size_t i = 0; i < states.size(); i++) {
                sendStateToJs(states[i], (int)i);
            }
            if (isInitFinished) {
                sendInitializedToJs();
            }
        }
    });
    runCallback(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Initializer::onJavascriptReady(const ReadyCallback &callback) {
BEGIN_SLOT_WRAPPER
    ReadyType result = ReadyType::Error;
    const TypedException exception = apiVrapper2([&, this] {
        if (isInitFinished) {
            for (std::unique_ptr<InitInterface> &init: initializiers) {
                init->complete();
            }
            result = ReadyType::Finish;
        } else {
            result = ReadyType::Advance;
        }
    });
    runCallback(std::bind(callback, result, exception));
END_SLOT_WRAPPER
}

}
