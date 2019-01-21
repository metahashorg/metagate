#include "InitializerJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"

#include "Initializer.h"

namespace initializer {

static QJsonDocument typesToJson(const std::vector<QString> &types) {
    QJsonArray messagesJson;
    for (const QString &type: types) {
        messagesJson.push_back(type);
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

    qRegisterMetaType<InitState>("InitState");
    qRegisterMetaType<TypedException>("TypedException");
}

void InitializerJavascript::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitializerJavascript::resendEvents() {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsResendEventsJs";

    LOG << "Resend events";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit m_initializer->resendAllStatesSig([makeFunc](const TypedException &exception) {
            makeFunc(exception);
        });
    });

    if (exception.isSet()) {
        makeFunc(exception);
    }
END_SLOT_WRAPPER
}

void InitializerJavascript::ready(bool force) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsReadyResultJs";

    LOG << "Javascript ready";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit m_initializer->javascriptReadySig(force, [makeFunc](const Initializer::ReadyType &result, const TypedException &exception) {
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
            } else {
                throwErr("Unknown type");
            }
            makeFunc(exception, r);
        });
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
    LOG << "getAllTypes";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit m_initializer->getAllTypes([makeFunc](const std::vector<QString> &result, const TypedException &exception) {
            makeFunc(exception, typesToJson(result));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void InitializerJavascript::onStateChanged(int number, int totalStates, int numberCritical, int totalCritical, const InitState &state) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initStateChangedJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, state.exception, number, totalStates, numberCritical, totalCritical, state.type, state.subType, state.message, state.isCritical, state.isScipped);
END_SLOT_WRAPPER
}

void InitializerJavascript::onInitialized(bool isSuccess, const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

    const QString JS_NAME_RESULT = "initInitializedJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, isSuccess);
END_SLOT_WRAPPER
}

void InitializerJavascript::onInitializedCritical(bool isSuccess, const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    CHECK(m_initializer != nullptr, "initializer not set");

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
