#include "Initializer.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QSettings>

#include "check.h"
#include "SlotWrapper.h"
#include "Paths.h"

#include "InitializerJavascript.h"
#include "InitInterface.h"

namespace initializer {

InitState::InitState(int number, const QString &type, const QString &subType, const QString &message, const TypedException &exception)
    : number(number)
    , type(type)
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
    CHECK(connect(this, &Initializer::sendState, this, &Initializer::onSendState), "not connect onSendState");

    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<GetAllStatesCallback>("GetAllStatesCallback");
    qRegisterMetaType<ReadyCallback>("ReadyCallback");
    qRegisterMetaType<InitState>("InitState");
}

Initializer::~Initializer() = default;

void Initializer::complete() {
    isComplete = true;
    if (isComplete && !isInitFinished && (int)states.size() == totalStates) {
        isInitFinished = true;
        sendInitializedToJs();
    }
}

template<typename Func>
void Initializer::runCallback(const Func &callback) {
    emit javascriptWrapper.callbackCall(callback);
}

void Initializer::sendStateToJs(const InitState &state) {
    emit javascriptWrapper.stateChangedSig(totalStates, state);
}

void Initializer::sendInitializedToJs() {
    emit javascriptWrapper.initializedSig(TypedException());
}

void Initializer::onSendState(const InitState &state) {
BEGIN_SLOT_WRAPPER
    CHECK(states.find(state.number) == states.end(), "State already setted");
    CHECK(0 <= state.number && state.number < totalStates, "Incorrect state number");
    states[state.number] = state;
    sendStateToJs(state);
    if (isComplete && !isInitFinished && (int)states.size() == totalStates) {
        isInitFinished = true;
        sendInitializedToJs();
    }
END_SLOT_WRAPPER
}

void Initializer::onResendAllStates(const GetAllStatesCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        for (const auto &statePair: states) {
            sendStateToJs(statePair.second);
        }
        if (isInitFinished) {
            sendInitializedToJs();
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
