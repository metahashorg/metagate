#ifndef PATHS_H
#define PATHS_H

#include <QString>

QString getWalletPath();

QString getLogPath();

QString getNsLookupPath();

QString getPagesPath();

QString getSettingsPath();

QString getAutoupdaterPath();

QString getTmpAutoupdaterPath();

void clearAutoupdatersPath();

#endif // PATHS_H
