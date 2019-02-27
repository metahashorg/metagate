#include "InitializerJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <functional>
using namespace std::placeholders;

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"
#include "QRegister.h"

#include "Initializer.h"

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

InitializerJavascript::InitializerJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &InitializerJavascript::callbackCall, this, &InitializerJavascript::onCallbackCall), "not connect onCallbackCall");

    CHECK(connect(this, &InitializerJavascript::stateChangedSig, this, &InitializerJavascript::onStateChanged), "not connect onStateChanged");
    CHECK(connect(this, &InitializerJavascript::initializedSig, this, &InitializerJavascript::onInitialized), "not connect onInitialized");
    CHECK(connect(this, &InitializerJavascript::initializedCriticalSig, this, &InitializerJavascript::onInitializedCritical), "not connect onInitializedCritical");

    Q_REG3(InitState, "InitState", "initialize");
    Q_REG2(TypedException, "TypedException", false);

    signalFunc = std::bind(&InitializerJavascript::callbackCall, this, _1);
}

void InitializerJavascript::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitializerJavascript::resendEvents() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initsResendEventsJs";

    LOG << "Init Resend events";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception);
    };

    const auto errorFunc = [makeFunc, this](const TypedException &exception) {
        makeFunc(exception);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit m_initializer->resendAllStatesSig(Initializer::GetAllStatesCallback([makeFunc]() {
            makeFunc(TypedException());
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception);
    }
END_SLOT_WRAPPER
}

void InitializerJavascript::ready(bool force) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initsReadyResultJs";

    LOG << "Init js ready " << force;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const auto errorFunc = [makeFunc, this](const TypedException &exception) {
        makeFunc(exception, "");
    };

    const TypedException exception = apiVrapper2([&, this](){
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
            makeFunc(TypedException(), r);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, "error");
    }
END_SLOT_WRAPPER
}

void InitializerJavascript::getAllTypes() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initGetAllTypesResultJs";
    LOG << "Init getAllTypes";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const auto errorFunc = [makeFunc, this](const TypedException &exception) {
        makeFunc(exception, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit m_initializer->getAllTypes(Initializer::GetTypesCallback([makeFunc](const std::vector<QString> &result) {
            LOG << "Init getAllTypes size " << result.size();
            makeFunc(TypedException(), typesToJson(result));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void InitializerJavascript::getAllSubTypes() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initGetAllSubTypesResultJs";
    LOG << "Init getAllSubTypes";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const auto errorFunc = [makeFunc, this](const TypedException &exception) {
        makeFunc(exception, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit m_initializer->getAllSubTypes(Initializer::GetSubTypesCallback([makeFunc](const std::vector<Initializer::StateType> &result) {
            LOG << "Init getAllSubTypes size " << result.size();
            makeFunc(TypedException(), subTypesToJson(result));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, QJsonDocument());
    }
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

template<typename... Args>
void InitializerJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void InitializerJavascript::runJs(const QString &script) {
    LOG << "Js " << script;
    emit jsRunSig(script);
}

}
