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

/*
ProxyStatus parseStatusMessage(const std::string &message) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(QByteArray(message.data(), message.size()));
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();

    if (json1.contains("error") && json1.value("error").isObject()) {
        const ErrorResponse errorJson = parseErrorResponse(json1);
        throwErr(errorJson.message.toStdString());
    } else {
        CHECK(json1.contains("result") && json1.value("result").isObject(), "result field not found");
        const QJsonObject resultJson = json1.value("result").toObject();

        CHECK(resultJson.contains("status") && resultJson.value("status").isString(), "status field not found");
        const QString statusStr = resultJson.value("status").toString();
        CHECK(resultJson.contains("description") && resultJson.value("description").isString(), "description field not found");
        const QString descriptionStr = resultJson.value("description").toString();

        ProxyStatus result;
        result.description = descriptionStr;
        if (statusStr == "started") {
            result.status = ProxyStatus::Status::started;
        } else if (statusStr == "begin_test") {
            result.status = ProxyStatus::Status::begin_test;
        } else if (statusStr == "error_begin_test") {
            result.status = ProxyStatus::Status::error_begin_test;
        } else if (statusStr == "success_test") {
            result.status = ProxyStatus::Status::success_test;
        } else if (statusStr == "failure_test") {
            result.status = ProxyStatus::Status::failure_test;
        } else {
            throwErr("Incorrect proxy status: " + statusStr.toStdString());
        }

        return result;
    }
}

RefreshConfigResponse parseRefreshConfigMessage(const std::string &message) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(QByteArray(message.data(), message.size()));
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();

    RefreshConfigResponse result;
    if (json1.contains("error") && json1.value("error").isObject()) {
        const ErrorResponse errorJson = parseErrorResponse(json1);
        result.isError = true;
        result.error = errorJson.message;
    }
    return result;

*/

} // namespace proxy_client
