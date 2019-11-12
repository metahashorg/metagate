#ifndef METAGATEMESSAGES_H
#define METAGATEMESSAGES_H

#include <QString>
#include <QUrl>

#include <vector>

namespace transactions {
struct BalanceInfo;
}

class QJsonDocument;

namespace metagate {

QString makeCommandLineMessageForWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

QString makeMessageApplicationForWss(const QString &hardwareId, const QString &utmData, const QString &userId, const QString &applicationVersion, const QString &interfaceVersion, bool isForgingActive, const std::vector<QString> &keysTmh, const std::vector<QString> &keysMth, bool isVirtualMachine, const QString &osName);

QString metaOnlineMessage();

QString makeTestTorrentResponse(const QString &id, bool res, const QString &descr, const std::vector<std::pair<QString, transactions::BalanceInfo>> &result);

QString parseAppType(const QJsonDocument &response);

QString parseMetaOnlineResponse(const QJsonDocument &response);

std::pair<QString, QString> parseShowExchangePopupResponse(const QJsonDocument &response);

QString parseTestTorrentRequest(const QJsonDocument &response, QUrl &url, std::vector<std::pair<QString, QString>> &addresses);

} // namespace metagate

#endif // METAGATEMESSAGES_H
