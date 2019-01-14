#include "WebSocketSender.h"

#include "WebSocketClient.h"

#include <vector>

#include "SlotWrapper.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace proxy {

static QString makeGetMyIpMessage() {
    QJsonObject allJson;
    allJson.insert("app", "GetMyIp");
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

static QString makeStartTestMessage(QString ip, int port) {
    QJsonObject allJson;
    allJson.insert("app", "ProxyStartTest");
    QJsonObject data;
    data.insert("ip", ip);
    data.insert("port", port);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
};

WebSocketSender::WebSocketSender(WebSocketClient &client, QObject *parent)
    : QObject(parent)
    , client(client)
{
    CHECK(connect(&client, &WebSocketClient::messageReceived, this, &WebSocketSender::onWssReceived), "not connect onWssReceived");

    emit client.setHelloString(std::vector<QString>(), "proxy");

    const QString getMyIpMessage = makeGetMyIpMessage();
    emit client.addHelloString(getMyIpMessage, "proxy");
    emit client.sendMessage(getMyIpMessage);
}

void WebSocketSender::onStartTest() {
BEGIN_SLOT_WRAPPER
    const QString startTestMessage = makeStartTestMessage(myIp, port);
    emit client.addHelloString(startTestMessage, "proxy");
    emit client.sendMessage(startTestMessage);
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
