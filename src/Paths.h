#ifndef PATHS_H
#define PATHS_H

#include <QString>

QString getWalletPath();

QString getLogPath();

QString getBdPath();

QString getNsLookupPath();

QString getPagesPath();

QString getSettingsPath();

QString getAutoupdaterPath();

QString getTmpAutoupdaterPath();

void clearAutoupdatersPath();

#endif // PATHS_H
