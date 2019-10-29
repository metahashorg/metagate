#include "MetaGateMessages.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "transactions/Transaction.h"

#include "Log.h"
#include "check.h"

SET_LOG_NAMESPACE("MG");

namespace metagate {

QString makeCommandLineMessageForWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText) {
    QJsonObject allJson;
    allJson.insert(QStringLiteral("app"), QStringLiteral("MetaSearch"));
    QJsonObject data;
    data.insert(QStringLiteral("machine_uid"), hardwareId);
    data.insert(QStringLiteral("user_id"), userId);
    data.insert(QStringLiteral("focus_release_count"), static_cast<int>(focusCount));
    data.insert(QStringLiteral("text"), QString(line.toUtf8().toHex()));
    data.insert(QStringLiteral("is_enter_pressed"), isEnter);
    data.insert(QStringLiteral("is_user_text"), isUserText);
    allJson.insert(QStringLiteral("data"), data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

QString makeMessageApplicationForWss(const QString &hardwareId, const QString &utmData, const QString &userId, const QString &applicationVersion, const QString &interfaceVersion, bool isForgingActive, const std::vector<QString> &keysTmh, const std::vector<QString> &keysMth, bool isVirtualMachine, const QString &osName) {
    QJsonObject allJson;
    allJson.insert(QStringLiteral("app"), QStringLiteral("MetaGate"));
    QJsonObject data;
    data.insert(QStringLiteral("machine_uid"), hardwareId);
    data.insert(QStringLiteral("utm_data"), utmData);
    data.insert(QStringLiteral("user_id"), userId);
    data.insert(QStringLiteral("application_ver"), applicationVersion);
    data.insert(QStringLiteral("interface_ver"), interfaceVersion);
    data.insert(QStringLiteral("is_virtual"), isVirtualMachine);
    data.insert(QStringLiteral("isForgingActive"), isForgingActive);
    data.insert(QStringLiteral("os_name"), osName);

    QJsonArray keysTmhJson;
    for (const QString &key: keysTmh) {
        keysTmhJson.push_back(key);
    }
    data.insert(QStringLiteral("keys_tmh"), keysTmhJson);

    QJsonArray keysMthJson;
    for (const QString &key: keysMth) {
        keysMthJson.push_back(key);
    }
    data.insert(QStringLiteral("keys_mth"), keysMthJson);

    allJson.insert(QStringLiteral("data"), data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

QString metaOnlineMessage() {
    return QStringLiteral("{\"app\":\"MetaOnline\"}");
}


QString makeTestTorrentResponse(const QString &id, bool res, const QString &descr, const std::vector<std::pair<QString, transactions::BalanceInfo>> &result)
{
    QJsonObject allJson;
    allJson.insert(QStringLiteral("app"), QStringLiteral("TestTorrent"));
    allJson.insert(QStringLiteral("id"), id);
    if (!res) {
        allJson.insert(QStringLiteral("res"), descr);
    }
    QJsonArray a;
    for (const std::pair<QString, transactions::BalanceInfo> &balance : result) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), balance.first);
        if (res) {
            obj.insert(QStringLiteral("addr"), balance.second.address);
            obj.insert(QStringLiteral("spent"), QString::fromLatin1(balance.second.spent.getDecimal()));
            obj.insert(QStringLiteral("received"), QString::fromLatin1(balance.second.received.getDecimal()));
            obj.insert(QStringLiteral("transactions"), QString::number(balance.second.countTxs));
        }
        a.push_back(obj);
    }
    allJson.insert(QStringLiteral("data"), a);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}


QString parseAppType(const QJsonDocument &response)
{
    const QJsonObject root = response.object();
    if (!root.contains(QLatin1String("app")) || !root.value(QLatin1String("app")).isString()) {
        return QString();
    }
    return root.value(QLatin1String("app")).toString();
}

QString parseMetaOnlineResponse(const QJsonDocument &response) {
    const QJsonObject root = response.object();
    if (!root.contains(QLatin1String("app")) || !root.value(QLatin1String("app")).isString()) {
        return "";
    }
    const QString appType = root.value(QLatin1String("app")).toString();

    if (appType == QLatin1String("MetaOnline")) {
        if (root.contains(QLatin1String("data")) && root.value(QLatin1String("data")).isObject()) {
            const QJsonObject data = root.value(QLatin1String("data")).toObject();
            return QJsonDocument(data).toJson(QJsonDocument::JsonFormat::Compact);
        };
    }
    return "";
}

std::pair<QString, QString> parseShowExchangePopupResponse(const QJsonDocument &response) {
    const QJsonObject root = response.object();
    if (!root.contains(QLatin1String("app")) || !root.value(QLatin1String("app")).isString()) {
        return std::make_pair(QString(), QString());
    }
    const QString appType = root.value(QLatin1String("app")).toString();
    if (appType == QStringLiteral("InEvent")) {
        const QString event = root.value(QLatin1String("event")).toString();
        if (event == QLatin1String("showExchangePopUp")) {
            const QString user = root.value(QLatin1String("user")).toString();
            const QString type = root.value(QLatin1String("type")).toString();
            return std::make_pair(user, type);
        }
    }
    return std::make_pair(QString(), QString());
}

QString parseTestTorrentRequest(const QJsonDocument &response, QUrl &url, std::vector<std::pair<QString, QString>> &addresses)
{
    QString id;
    const QJsonObject root = response.object();
    if (!root.contains(QLatin1String("app")) || !root.value(QLatin1String("app")).isString()) {
        return id;
    }
    const QString appType = root.value(QLatin1String("app")).toString();
    if (appType == QLatin1String("TestTorrent")) {
        CHECK(root.contains(QLatin1String("id")) && root.value(QLatin1String("id")).isString(), "id field not found");
        id = root.value(QLatin1String("id")).toString();
        CHECK(root.contains(QLatin1String("url")) && root.value(QLatin1String("url")).isString(), "url field not found");
        url = QUrl(root.value(QLatin1String("url")).toString());
        CHECK(root.contains(QLatin1String("addresses")) && root.value(QLatin1String("addresses")).isArray(), "addresses field not found");
        const QJsonArray addrs = root.value(QLatin1String("addresses")).toArray();
        for (int i = 0; i < addrs.count(); i++) {
            CHECK(addrs[i].isObject(), "address array field is not object");
            const QJsonObject o = addrs[i].toObject();
            CHECK(o.contains(QLatin1String("id")) && o.value(QLatin1String("id")).isString(), "id field not found");
            CHECK(o.contains(QLatin1String("addr")) && o.value(QLatin1String("addr")).isString(), "addr field not found");
            const QString id = o.value(QLatin1String("id")).toString();
            const QString addr = o.value(QLatin1String("addr")).toString();
            addresses.emplace_back(std::make_pair(id, addr));
        }
    }
    return  id;
}

} // namespace metagate
