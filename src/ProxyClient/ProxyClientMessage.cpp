#include "ProxyClientMessage.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("PXC");

namespace proxy_client {

struct ErrorResponse {
    int code;
    QString message;
};

QString makeGetStatusMessage() {
    return "{\"method\": \"status\"}";
}

QString makeRefreshConfigMessage() {
    return "{\"method\": \"refreshConfig\"}";
}

static ErrorResponse parseErrorResponse(const QJsonObject &json) {
    CHECK(json.contains("error") && json.value("error").isObject(), "error field not found");
    ErrorResponse result;

    const QJsonObject errorJson = json.value("error").toObject();
    CHECK(errorJson.contains("message") && errorJson.value("message").isString(), "Incorrect json: error/message field not found");
    result.message = errorJson.value("message").toString();
    CHECK(errorJson.contains("code") && errorJson.value("code").isDouble(), "Incorrect json: error/code field not found");
    result.code = errorJson.value("code").toInt();

    return result;
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
}

} // namespace proxy_client
