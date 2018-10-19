#include "Auth.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QSettings>

#include "check.h"
#include "SlotWrapper.h"
#include "Paths.h"

#include "AuthJavascript.h"

#include "machine_uid.h"

namespace auth
{

const QString AuthURL = "http://id.metahash.org/api/";


Auth::Auth(AuthJavascript &javascriptWrapper, QObject *parent)
    : TimerClass(1min, parent)
    , javascriptWrapper(javascriptWrapper)
{
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    authUrl = settings.value("servers/auth", AuthURL).toString();
    hardwareId = QString::fromStdString(::getMachineUid());

    readLoginInfo();

    CHECK(connect(this, &Auth::timerEvent, this, &Auth::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &Auth::startedEvent, this, &Auth::onStarted), "not connect onStarted");

    CHECK(connect(this, &Auth::login, this, &Auth::onLogin), "not connect onLogin");
    CHECK(connect(this, &Auth::logout, this, &Auth::onLogout), "not connect onLogout");
    CHECK(connect(this, &Auth::check, this, &Auth::onCheck), "not connect onCheck");

    qRegisterMetaType<LoginInfo>("LoginInfo");

    tcpClient.setParent(this);
    CHECK(connect(&tcpClient, &SimpleClient::callbackCall, this, &Auth::onCallbackCall), "not connect");
    tcpClient.moveToThread(&thread1);

    moveToThread(&thread1); // TODO вызывать в TimerClass
}

void Auth::onLogin(const QString &login, const QString &password)
{
BEGIN_SLOT_WRAPPER
    qDebug() << "START";
    const TypedException exception = apiVrapper2([&, this] {
        const QString request = makeLoginRequest(login, password);
        qDebug() << request;
        tcpClient.sendMessagePost(authUrl, request, [this, login](const std::string &response, const TypedException &error) {
            if (error.isSet()) {
                qDebug() << "Login error";
                emit javascriptWrapper.sendLoginErrorResponseSig(error);
            } else {
                qDebug() << "RES " << QString::fromStdString(response);
                const TypedException exception = apiVrapper2([&] {
                info = parseLoginResponse(QString::fromStdString(response));
                info.login = login;
                writeLoginInfo();

                });
                emit logined();
                emit javascriptWrapper.sendLoginInfoResponseSig(info, exception);
            }
        });
    });
END_SLOT_WRAPPER
}

void Auth::onLogout(/*const Auth::LoginCallback &callback*/)
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        doLogout();
    });
    emit javascriptWrapper.sendLoginInfoResponseSig(info, exception);
END_SLOT_WRAPPER
}

void Auth::onCheck(/*const Auth::LoginCallback &callback*/)
{
    emit javascriptWrapper.sendLoginInfoResponseSig(info, TypedException());
}

void auth::Auth::onCallbackCall(Callback callback)
{
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void auth::Auth::onStarted()
{
    qDebug() << "auth::Auth::onStarted()";
    checkToken();
}

void auth::Auth::onTimerEvent()
{
    qDebug() << "auth::Auth::onTimerEvent()";
    checkToken();
}

void auth::Auth::doLogout()
{
    info.clear();
    writeLoginInfo();
    emit logouted();
}

void auth::Auth::readLoginInfo()
{
    QSettings settings(getStoragePath(), QSettings::IniFormat);
    settings.beginGroup("Login");
    info.login = settings.value("login", QString()).toString();
    info.token = settings.value("token", QString()).toString();
    info.isAuth = settings.value("isAuth", false).toBool();
    info.isTest = settings.value("isTest", false).toBool();
    settings.endGroup();
}

void auth::Auth::writeLoginInfo()
{
    QSettings settings(getStoragePath(), QSettings::IniFormat);
    settings.beginGroup("Login");
    settings.setValue("login", info.login);
    settings.setValue("token", info.token);
    settings.setValue("isAuth", info.isAuth);
    settings.setValue("isTest", info.isTest);
    settings.endGroup();
}

void auth::Auth::checkToken()
{
    if (!info.isAuth)
        return;
    const TypedException exception = apiVrapper2([&, this] {
        const QString request = makeCheckTokenRequest(info.token);
        qDebug() << request;
        tcpClient.sendMessagePost(authUrl, request, [this](const std::string &response, const TypedException &error) {
            qDebug() << "RES1 " << QString::fromStdString(response);
            if (error.isSet()) {
                logout();
                emit javascriptWrapper.sendLoginInfoResponseSig(info, error);
            }
            else {
                const TypedException exception = apiVrapper2([&] {
                    bool res = parseCheckTokenResponse(QString::fromStdString(response));
                    if (!res)
                        logout();
                });
                emit javascriptWrapper.sendLoginInfoResponseSig(info, exception);
            }
        });
    });
}

template<typename Func>
void Auth::runCallback(const Func &callback)
{
    emit javascriptWrapper.callbackCall(callback);
}

QString Auth::makeLoginRequest(const QString &login, const QString &password)
{
    QJsonObject request;
    request.insert("id", "1");
    request.insert("version", "1.0.0");
    request.insert("method", "user.auth");
    request.insert("uid", hardwareId);
    QJsonArray params;
    QJsonObject p;
    p.insert("login", login);
    p.insert("password", password);
    params.append(p);

    request.insert("params", params);
    return QString(QJsonDocument(request).toJson(QJsonDocument::Compact));
}

QString auth::Auth::makeCheckTokenRequest(const QString &token)
{
    QJsonObject request;
    request.insert("id", "1");
    request.insert("version", "1.0.0");
    request.insert("method", "user.token");
    request.insert("token", token);
    request.insert("uid", hardwareId);
    QJsonObject params;
    request.insert("params", params);
    return QString(QJsonDocument(request).toJson(QJsonDocument::Compact));
}

LoginInfo Auth::parseLoginResponse(const QString &response)
{
    LoginInfo result;
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json");
    const QJsonObject &json1 = jsonResponse.object();

    CHECK(json1.contains("result") && json1.value("result").isString(), "Incorrect json: result field not found");

    //result: "OK",

    CHECK(json1.contains("data") && json1.value("data").isObject(), "Incorrect json: data field not found");
    const QJsonObject &json = json1.value("data").toObject();

    CHECK(json.contains("token") && json.value("token").isString(), "Incorrect json: token field not found");
    result.token = json.value("token").toString();

    CHECK(json.contains("is_test_user") && json.value("is_test_user").isBool(), "Incorrect json: is_test_user field not found");
    result.isTest = json.value("is_test_user").toBool();

    result.isAuth = result.token.isEmpty() ? false : true;

    return result;
}

bool auth::Auth::parseCheckTokenResponse(const QString &response)
{
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json");
    const QJsonObject &json = jsonResponse.object();

    CHECK(json.contains("result") && json.value("result").isString(), "Incorrect json: result field not found");

    return (json.value("result").toString() == QStringLiteral("OK"));
}

}
