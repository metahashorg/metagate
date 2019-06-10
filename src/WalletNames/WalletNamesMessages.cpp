#include "WalletNamesMessages.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "utils.h"
#include "check.h"

namespace wallet_names {

const static QString RENAME_METHOD = "rename";
const static QString SET_WALLETS_METHOD = "set-wallets";
const static QString GET_WALLETS_METHOD = "get-wallets";

static void addHeaderToJson(QJsonObject &json, size_t id, const QString &token, const QString &hwid) {
    json.insert("jsonrpc", "2.0");
    json.insert("token", token);
    json.insert("machine_id", hwid);
    json.insert("request_id", QString::fromStdString(std::to_string(id)));
}

QString makeRenameMessage(const QString &address, const QString &name, size_t id, const QString &token, const QString &hwid) {
    QJsonObject json;
    addHeaderToJson(json, id, token, hwid);
    json.insert("method", RENAME_METHOD);
    QJsonObject params;
    params.insert("address", address);
    params.insert("name", toHex(name));
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeSetWalletsMessage(const std::vector<WalletInfo> &infos, size_t id, const QString &token, const QString &hwid) {
    QJsonObject json;
    addHeaderToJson(json, id, token, hwid);
    json.insert("method", SET_WALLETS_METHOD);
    QJsonArray params;
    for (const WalletInfo &info: infos) {
        QJsonObject param;
        param.insert("address", info.address);
        param.insert("name", toHex(info.name));

        QJsonArray locations;
        for (const WalletInfo::Info &i: info.infos) {
            QJsonObject location;
            location.insert("user", toHex(i.user));
            location.insert("device", i.device);
            location.insert("currency", i.currency);

            locations.push_back(location);
        }
        param.insert("locations", locations);
        params.push_back(param);
    }
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeGetWalletsMessage(size_t id, const QString &token, const QString &hwid) {
    QJsonObject json;
    addHeaderToJson(json, id, token, hwid);
    json.insert("method", GET_WALLETS_METHOD);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

ResponseType getMethodAndAddressResponse(const QJsonDocument &response) {
    ResponseType result;
    QString type;

    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    if (root.contains("method") && root.value("method").isString()) {
        type = root.value("method").toString();
    }
    if (root.contains("error") && root.value("error").isString()) {
        result.error = root.value("error").toString();
        result.isError = true;
        /*} else {
            result.errorType = ResponseType::ERROR_TYPE::OTHER;
        }*/
        if (root.contains("data")) {
            QJsonObject obj;
            obj.insert("data", root.value("data"));
            result.error += ". " + QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Compact);
        }
    }
    if (root.contains("request_id") && root.value("request_id").isString()) {
        result.id = std::stoull(root.value("request_id").toString().toStdString());
    }

    if (type == RENAME_METHOD) {
        result.method = METHOD::RENAME;
    } else if (type == SET_WALLETS_METHOD) {
        result.method = METHOD::SET_WALLETS;
    } else if (type == GET_WALLETS_METHOD) {
        result.method = METHOD::GET_WALLETS;
    } else if (type.isEmpty()) {
        // ignore
    } else {
        result.method = METHOD::ALIEN;
    }
    return result;
}

std::vector<WalletInfo> parseGetWalletsMessage(const QJsonDocument &response) {
    std::vector<WalletInfo> result;

    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    CHECK(root.contains("data") && root.value("data").isArray(), "data field not found");
    const QJsonArray data = root.value("data").toArray();
    for (const QJsonValue &walletJson: data) {
        CHECK(walletJson.isObject(), "wallets field not found");
        const QJsonObject &walletJsonObj = walletJson.toObject();

        WalletInfo wallet;
        CHECK(walletJsonObj.contains("address") && walletJsonObj.value("address").isString(), "address field not found");
        wallet.address = walletJsonObj.value("address").toString();
        CHECK(walletJsonObj.contains("name") && walletJsonObj.value("name").isString(), "name field not found");
        wallet.name = fromHex(walletJsonObj.value("name").toString());

        if (walletJsonObj.contains("locations") && walletJsonObj.value("locations").isArray()) {
            const QJsonArray locations = walletJsonObj.value("locations").toArray();
            for (const QJsonValue &location: locations) {
                CHECK(location.isObject(), "locations field not found");
                const QJsonObject &locationObj = location.toObject();

                WalletInfo::Info info;
                CHECK(locationObj.contains("user") && locationObj.value("user").isString(), "user field not found");
                info.user = fromHex(locationObj.value("user").toString());
                CHECK(locationObj.contains("device") && locationObj.value("device").isString(), "device field not found");
                info.device = locationObj.value("device").toString();
                CHECK(locationObj.contains("currency") && locationObj.value("currency").isString(), "currency field not found");
                info.currency = locationObj.value("currency").toString();

                wallet.infos.emplace_back(info);
            }
        }

        result.emplace_back(wallet);
    }

    return result;
}

void parseAddressListResponse(const QString &response, QStringList &tmhs, QStringList &mhcs)
{
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(json1.contains("result") && json1.value("result").isString() && json1.value("result").toString() == QStringLiteral("OK"), "Incorrect json: result field not found or wrong");
    CHECK(json1.contains("data") && json1.value("data").isArray(), "Incorrect json: data field not found");
    const QJsonArray &json = json1.value("data").toArray();

    QStringList result;

    for (const QJsonValue &elementJson: json) {
        CHECK(elementJson.isObject(), "Incorrect json");
        const QJsonObject walJson = elementJson.toObject();

        CHECK(walJson.contains("address") && walJson.value("address").isString(), "Incorrect json: address field not found");
        const QString address = walJson.value("address").toString();
        CHECK(walJson.contains("read_only") && walJson.value("read_only").isBool(), "Incorrect json: read_only field not found");
        const bool readOnly = walJson.value("read_only").toBool();
        CHECK(walJson.contains("currency_code") && walJson.value("currency_code").isString(), "Incorrect json: currency_code field not found");
        const QString currency = walJson.value("currency_code").toString();
        if (readOnly) {
            if (currency == QStringLiteral("MHC"))
                mhcs.append(address);
            if (currency == QStringLiteral("TMH"))
                tmhs.append(address);
        }
    }
}

} // namespace wallet_names
