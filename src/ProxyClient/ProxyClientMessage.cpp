#include "ProxyClientMessage.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("PXC");

namespace proxy_client {

QByteArray makeGetStatusMessage() {
    return QByteArray("{\"method\": \"status\"}");
}

QByteArray makeRefreshConfigMessage() {
    return QByteArray("{\"method\": \"restart\"}");
}

ProxyResponse parseProxyResponse(const QByteArray &message)
{
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(message);
    CHECK(jsonResponse.isObject(), "Incorrect json");
    const QJsonObject &root = jsonResponse.object();

    ProxyResponse result;

    CHECK(root.contains(QLatin1String("result")) && root.value(QLatin1String("result")).isString(), "result field not found");
    const QString rt = root.value(QLatin1String("result")).toString();
    if (rt == QStringLiteral("OK"))
        result.error = false;
    else
        result.error = true;
    return result;
}

QString parseProxyStatusResponse(const QByteArray &message)
{
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(message);
    CHECK(jsonResponse.isObject(), "Incorrect json");

    const QJsonObject root = jsonResponse.object();
    CHECK(root.contains("params") && root.value("params").isObject(), "params field not found");
    const QJsonObject params = root.value("params").toObject();

    return QString(QJsonDocument(params).toJson(QJsonDocument::Compact));
}

} // namespace proxy_client
