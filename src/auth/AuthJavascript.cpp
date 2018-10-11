#include "AuthJavascript.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"

#include "Auth.h"

namespace auth
{

static QJsonDocument loginInfoToJson(const LoginInfo &info)
{
    QJsonObject obj;
    obj["token"] = info.token;
    obj["is_auth"] = info.isAuth;
    obj["is_test"] = info.isTest;
    return QJsonDocument(obj);
}

AuthJavascript::AuthJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &AuthJavascript::callbackCall, this, &AuthJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &AuthJavascript::sendLoginInfoResponseSig, this, &AuthJavascript::onSendLoginInfoResponseSig), "not connect onSendLoginInfoResponseSig");
    CHECK(connect(this, &AuthJavascript::sendLoginErrorResponseSig, this, &AuthJavascript::onSendLoginErrorResponseSig), "not connect onSendLoginErrorResponseSig");
}

void AuthJavascript::login(const QString &login, const QString &password)
{
BEGIN_SLOT_WRAPPER
    CHECK(authManager, "auth not set");
    LOG << "Login " << login;

    const TypedException exception = apiVrapper2([&, this]()
    {
        emit authManager->login(login, password);
    });

END_SLOT_WRAPPER
}

void AuthJavascript::logout()
{
BEGIN_SLOT_WRAPPER
    CHECK(authManager, "auth not set");

    const TypedException exception = apiVrapper2([&, this]()
    {
        emit authManager->logout();
    });
END_SLOT_WRAPPER
}

void AuthJavascript::check()
{
BEGIN_SLOT_WRAPPER
    CHECK(authManager, "auth not set");

    const TypedException exception = apiVrapper2([&, this]()
    {
        emit authManager->check();
    });
END_SLOT_WRAPPER
}

void AuthJavascript::onSendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "authLoginInfoJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, error, loginInfoToJson(response));
END_SLOT_WRAPPER
}

void AuthJavascript::onSendLoginErrorResponseSig(const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "authLoginErrorJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, error, QString());
END_SLOT_WRAPPER
}

void AuthJavascript::onCallbackCall(const Callback &callback)
{
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

template<typename... Args>
void AuthJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void AuthJavascript::runJs(const QString &script)
{
    emit jsRunSig(script);
}

}
