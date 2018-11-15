#ifndef PATHS_H
#define PATHS_H

#include <QString>

QString getWalletPath();

QString getLogPath();

QString getDbPath();

QString getNsLookupPath();

QString getPagesPath();

QString getSettingsPath();

QString getStoragePath();

QString getAutoupdaterPath();

QString getTmpAutoupdaterPath();

void clearAutoupdatersPath();

void initializeAllPaths();

#endif // PATHS_H
