#ifndef PATHS_H
#define PATHS_H

#include <QString>

QString getWalletPath();

QString getLogPath();

QString getDbPath();

QString getNsLookupPath();

QString getPagesPath();

QString getSettingsPath();

QString getSettings2Path();

QString getAutoupdaterPath();

QString getTmpAutoupdaterPath();

void clearAutoupdatersPath();

#endif // PATHS_H
