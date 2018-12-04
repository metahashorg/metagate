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

    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<GetAllStatesCallback>("GetAllStatesCallback");
    qRegisterMetaType<ReadyCallback>("ReadyCallback");
}

void Initializer::complete() {
    isComplete = true;
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

void Initializer::sendState(const InitState &state) {
    CHECK(states.find(state.number) == states.end(), "State already setted");
    CHECK(0 <= state.number && state.number < totalStates, "Incorrect state number");
    states[state.number] = state;
    sendStateToJs(state);
    if (isComplete && (int)states.size() == totalStates) {
        isInitFinished = true;
        sendInitializedToJs();
    }
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
    const TypedException exception = apiVrapper2([&, this] {
    });
    runCallback(std::bind(callback, exception));
END_SLOT_WRAPPER
}

}
