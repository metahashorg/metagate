#include "AuthJavascript.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/makeJsFunc.h"
#include "qt_utilites/QRegister.h"

#include "Auth.h"

#include "qt_utilites/WrapperJavascriptImpl.h"

SET_LOG_NAMESPACE("AUTH");

namespace auth {

static QJsonDocument loginInfoToJson(const LoginInfo &info) {
    QJsonObject obj;
    obj["login"] = info.login;
    obj["token"] = info.token;
    obj["is_auth"] = info.isAuth;
    obj["is_test"] = info.isTest;
    return QJsonDocument(obj);
}

AuthJavascript::AuthJavascript(QObject *parent)
    : WrapperJavascript(false, LOG_FILE)
{
    Q_CONNECT(this, &AuthJavascript::sendLoginInfoResponseSig, this, &AuthJavascript::onSendLoginInfoResponseSig);
}

void AuthJavascript::login(const QString &login, const QString &password) {
BEGIN_SLOT_WRAPPER
    CHECK(m_authManager, "auth not set");
    LOG << "Login " << login;
    const TypedException exception = apiVrapper2([&, this]() {
        emit m_authManager->login(login, password);
    });
    if (exception.isSet()) {
        emit sendLoginInfoResponseSig(LoginInfo(), exception);
    }
END_SLOT_WRAPPER
}

void AuthJavascript::logout() {
BEGIN_SLOT_WRAPPER
    CHECK(m_authManager, "auth not set");

    LOG << "logout";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_authManager->logout();
    });
    if (exception.isSet()) {
        emit sendLoginInfoResponseSig(LoginInfo(), exception);
    }
END_SLOT_WRAPPER
}

void AuthJavascript::check() {
BEGIN_SLOT_WRAPPER
    CHECK(m_authManager, "auth not set");

    LOG << "get token";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_authManager->check();
    });
    if (exception.isSet()) {
        emit sendLoginInfoResponseSig(LoginInfo(), exception);
    }
END_SLOT_WRAPPER
}

void AuthJavascript::forceRefresh() {
BEGIN_SLOT_WRAPPER
    CHECK(m_authManager, "auth not set");

    LOG << "Force refresh";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_authManager->forceRefresh();
    });
    if (exception.isSet()) {
        emit sendLoginInfoResponseSig(LoginInfo(), exception);
    }
END_SLOT_WRAPPER
}

void AuthJavascript::onSendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "authLoginInfoJs";

    LOG << "Logined: " << response.login << ". Is test: " << response.isTest;

    makeAndRunJsFuncParams(JS_NAME_RESULT, error, loginInfoToJson(response));
END_SLOT_WRAPPER
}

}
