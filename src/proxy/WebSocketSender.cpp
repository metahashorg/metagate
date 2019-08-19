#include "WebSocketSender.h"

#include "Network/WebSocketClient.h"
#include "Proxy.h"

#include <vector>

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

#include <QJsonDocument>
#include <QJsonObject>

SET_LOG_NAMESPACE("PRX");

namespace proxy {

const static QString PROXY_TAG = "proxy";

static QString makeGetMyIpMessage() {
    QJsonObject allJson;
    allJson.insert("app", "GetMyIp");
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeProxyModuleFound() {
    QJsonObject allJson;
    allJson.insert("app", "ProxyModuleFound");
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeProxyBeginStart() {
    QJsonObject allJson;
    allJson.insert("app", "ProxyBeginStart");
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeStopProxy() {
    QJsonObject allJson;
    allJson.insert("app", "ProxyStopped");
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeProxyStarted(const TypedException &exception) {
    QJsonObject allJson;
    allJson.insert("app", "ProxyStarted");
    QJsonObject data;
    data.insert("errorNum", exception.numError);
    data.insert("errorMessage", QString::fromStdString(exception.description));
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeUpnpStarted(const TypedException &exception) {
    QJsonObject allJson;
    allJson.insert("app", "ProxyUpnpStarted");
    QJsonObject data;
    data.insert("errorNum", exception.numError);
    data.insert("errorMessage", QString::fromStdString(exception.description));
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeProxyStartCompleted(quint16 port) {
    QJsonObject allJson;
    allJson.insert("app", "ProxyStartCompleted");
    QJsonObject data;
    data.insert("port", port);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeStartTestMessage(const QString &ip, int port) {
    QJsonObject allJson;
    allJson.insert("app", "ProxyStartTest");
    QJsonObject data;
    data.insert("ip", ip);
    data.insert("port", port);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeAllCompleteMessage(const QString &ip, int port, int errorNum, const QString &errorMessage) {
    QJsonObject allJson;
    allJson.insert("app", "ProxyAllComplete");
    QJsonObject data;
    data.insert("ip", ip);
    data.insert("port", port);
    data.insert("errorNum", errorNum);
    data.insert("errorMessage", errorMessage);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

WebSocketSender::WebSocketSender(Proxy &proxyManager, QObject *parent)
    : QObject(parent)
    , client("wss://wss.osenyndab.com/")
    , proxyManager(proxyManager)
{
    Q_CONNECT(&client, &WebSocketClient::messageReceived, this, &WebSocketSender::onWssReceived);

    Q_CONNECT(this, &WebSocketSender::tryStartTest, this, &WebSocketSender::onTryStartTest);

    Q_CONNECT(&proxyManager, &Proxy::startAutoExecued, this, &WebSocketSender::onBeginStart);
    Q_CONNECT(&proxyManager, &Proxy::startAutoProxyResult, this, &WebSocketSender::onStartAutoProxyResult);
    Q_CONNECT(&proxyManager, &Proxy::startAutoUPnPResult, this, &WebSocketSender::onStartAutoUPnPResult);
    Q_CONNECT(&proxyManager, &Proxy::startAutoReadyToTest, this, &WebSocketSender::onReadyToTest);
    Q_CONNECT(&proxyManager, &Proxy::stopProxyExecuted, this, &WebSocketSender::onStopProxy);
    Q_CONNECT(this, &WebSocketSender::testResult, this, &WebSocketSender::onTestResult);
    Q_CONNECT(this, &WebSocketSender::proxyTested, &proxyManager, &Proxy::proxyTested);

    client.start();
    //client.moveToThread(thread);

    emit client.setHelloString(std::vector<QString>(), PROXY_TAG);

    const QString getMyIpMessage = makeGetMyIpMessage();
    emit client.addHelloString(getMyIpMessage, PROXY_TAG);
    emit client.sendMessage(getMyIpMessage);

    const QString getProxyModuleFound = makeProxyModuleFound();
    emit client.addHelloString(getProxyModuleFound, PROXY_TAG);
    emit client.sendMessage(getProxyModuleFound);
}

void WebSocketSender::onTryStartTest() {
BEGIN_SLOT_WRAPPER
    if (startComplete && !myIp.isEmpty()) {
        const QString startTestMessage = makeStartTestMessage(myIp, port);
        emit client.addHelloString(startTestMessage, PROXY_TAG);
        emit client.sendMessage(startTestMessage);
    }
END_SLOT_WRAPPER
}

void WebSocketSender::onBeginStart() {
BEGIN_SLOT_WRAPPER
    const QString message = makeProxyBeginStart();
    emit client.addHelloString(message, PROXY_TAG);
    emit client.sendMessage(message);
END_SLOT_WRAPPER
}

void WebSocketSender::onStartAutoProxyResult(const TypedException &r) {
BEGIN_SLOT_WRAPPER
    const QString message = makeProxyStarted(r);
    emit client.addHelloString(message, PROXY_TAG);
    emit client.sendMessage(message);
END_SLOT_WRAPPER
}

void WebSocketSender::onStartAutoUPnPResult(const TypedException &r) {
BEGIN_SLOT_WRAPPER
    const QString message = makeUpnpStarted(r);
    emit client.addHelloString(message, PROXY_TAG);
    emit client.sendMessage(message);
END_SLOT_WRAPPER
}

void WebSocketSender::onReadyToTest(quint16 port) {
BEGIN_SLOT_WRAPPER
    this->port = port;
    const QString message = makeProxyStartCompleted(port);
    emit client.addHelloString(message, PROXY_TAG);
    emit client.sendMessage(message);
    startComplete = true;

    emit tryStartTest();
END_SLOT_WRAPPER
}

void WebSocketSender::onStopProxy() {
BEGIN_SLOT_WRAPPER
    const QString message = makeStopProxy();
    emit client.addHelloString(message, PROXY_TAG);
    emit client.sendMessage(message);
END_SLOT_WRAPPER
}

void WebSocketSender::onTestResult(int code, QString result) {
BEGIN_SLOT_WRAPPER
    emit proxyTested(code == 0, result);

    const QString message = makeAllCompleteMessage(myIp, port, code, result);
    emit client.addHelloString(message, PROXY_TAG);
    emit client.sendMessage(message);
END_SLOT_WRAPPER
}

void WebSocketSender::onWssReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    CHECK(document.isObject(), "Message not is object");
    const QJsonObject root = document.object();
    CHECK(root.contains("app") && root.value("app").isString(), "app field not found");
    const std::string appType = root.value("app").toString().toStdString();

    if (appType == "GetMyIp") {
        CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
        const QJsonObject data = root.value("data").toObject();
        CHECK(data.contains("ip") && data.value("ip").isString(), "ip field not found");
        myIp = data.value("ip").toString();
        LOG << "Proxy My ip is " << myIp;

        emit tryStartTest();
    } else if (appType == "ProxyStartTest") {
        CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
        const QJsonObject data = root.value("data").toObject();
        CHECK(data.contains("code") && data.value("code").isDouble(), "code field not found");
        const int code = data.value("code").toInt();
        CHECK(data.contains("result") && data.value("result").isString(), "result field not found");
        const QString result = data.value("result").toString();

        emit testResult(code, result);
    }
END_SLOT_WRAPPER
}

}
