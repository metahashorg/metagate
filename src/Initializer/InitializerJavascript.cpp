#include "InitializerJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <functional>
using namespace std::placeholders;

#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/makeJsFunc.h"
#include "qt_utilites/QRegister.h"

#include "Initializer.h"

#include "qt_utilites/WrapperJavascriptImpl.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

static QJsonDocument typesToJson(const std::vector<QString> &types) {
    QJsonArray messagesJson;
    for (const QString &type: types) {
        messagesJson.push_back(type);
    }

    return QJsonDocument(messagesJson);
}

static QJsonDocument subTypesToJson(const std::vector<Initializer::StateType> &types) {
    QJsonArray messagesJson;
    for (const Initializer::StateType &type: types) {
        QJsonObject messageJson;
        messageJson.insert("type", type.type);
        messageJson.insert("subType", type.subtype);
        messageJson.insert("message", type.message);
        messageJson.insert("isCritical", type.isCritical);
        messagesJson.push_back(messageJson);
    }

    return QJsonDocument(messagesJson);
}

InitializerJavascript::InitializerJavascript()
    : WrapperJavascript(false, LOG_FILE)
{
    Q_CONNECT(this, &InitializerJavascript::stateChangedSig, this, &InitializerJavascript::onStateChanged);
    Q_CONNECT(this, &InitializerJavascript::initializedSig, this, &InitializerJavascript::onInitialized);
    Q_CONNECT(this, &InitializerJavascript::initializedCriticalSig, this, &InitializerJavascript::onInitializedCritical);

    Q_REG3(InitState, "InitState", "initialize");
    Q_REG2(TypedException, "TypedException", false);
}

void InitializerJavascript::resendEvents() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initsResendEventsJs";

    LOG << "Init Resend events";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT);

    wrapOperation([&, this](){
        emit m_initializer->resendAllStatesSig(Initializer::GetAllStatesCallback([makeFunc]() {
            makeFunc.func(TypedException());
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void InitializerJavascript::ready(bool force) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initsReadyResultJs";

    LOG << "Init js ready " << force;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit m_initializer->javascriptReadySig(force, Initializer::ReadyCallback([makeFunc](const Initializer::ReadyType &result) {
            QString r;
            if (result == Initializer::ReadyType::Error) {
                r = "error";
            } else if (result == Initializer::ReadyType::Finish) {
                r = "finish";
            } else if (result == Initializer::ReadyType::Advance) {
                r = "advance";
            } else if (result == Initializer::ReadyType::CriticalAdvance) {
                r = "critical_advance";
            } else if (result == Initializer::ReadyType::NotSuccess) {
                r = "not_success";
            } else if (result == Initializer::ReadyType::NotSuccessCritical) {
                r = "not_success_critical";
            } else {
                throwErr("Unknown type");
            }
            LOG << "Init js ready type " << r;
            makeFunc.func(TypedException(), r);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void InitializerJavascript::getAllTypes() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initGetAllTypesResultJs";
    LOG << "Init getAllTypes";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit m_initializer->getAllTypes(Initializer::GetTypesCallback([makeFunc](const std::vector<QString> &result) {
            LOG << "Init getAllTypes size " << result.size();
            makeFunc.func(TypedException(), typesToJson(result));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void InitializerJavascript::getAllSubTypes() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initGetAllSubTypesResultJs";
    LOG << "Init getAllSubTypes";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit m_initializer->getAllSubTypes(Initializer::GetSubTypesCallback([makeFunc](const std::vector<Initializer::StateType> &result) {
            LOG << "Init getAllSubTypes size " << result.size();
            makeFunc.func(TypedException(), subTypesToJson(result));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void InitializerJavascript::onStateChanged(int number, int totalStates, int numberCritical, int totalCritical, const InitState &state) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    LOG << "State sended " << number << " " << totalStates << " " << numberCritical << " " << totalCritical << " " << state.type << " " << state.subType << " " << state.exception.numError << " " << state.exception.description;
    const QString JS_NAME_RESULT = "initStateChangedJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, state.exception, number, totalStates, numberCritical, totalCritical, state.type, state.subType, state.message, state.isCritical, state.isScipped);
END_SLOT_WRAPPER
}

void InitializerJavascript::onInitialized(bool isSuccess, const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    LOG << "Initialized sended " << isSuccess;
    const QString JS_NAME_RESULT = "initInitializedJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, isSuccess);
END_SLOT_WRAPPER
}

void InitializerJavascript::onInitializedCritical(bool isSuccess, const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    LOG << "Critical initialized sended " << isSuccess;
    const QString JS_NAME_RESULT = "initInitializedCriticalJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, isSuccess);
END_SLOT_WRAPPER
}

}
