#ifndef METAGATEMESSAGES_H
#define METAGATEMESSAGES_H

#include <QString>

#include <vector>

class QJsonDocument;

namespace metagate {

QString makeCommandLineMessageForWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

QString makeMessageApplicationForWss(const QString &hardwareId, const QString &utmData, const QString &userId, const QString &applicationVersion, const QString &interfaceVersion, bool isForgingActive, const std::vector<QString> &keysTmh, const std::vector<QString> &keysMth, bool isVirtualMachine, const QString &osName);

QString metaOnlineMessage();

QString parseMetaOnlineResponse(const QJsonDocument &response);

std::pair<QString, QString> parseShowExchangePopupResponse(const QJsonDocument &response);

} // namespace metagate

#endif // METAGATEMESSAGES_H
