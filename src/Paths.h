#ifndef PATHS_H
#define PATHS_H

#include <QString>

QString getWalletPath();

QString getLogPath();

QString getDbPath();

QString getNsLookupPath();

QString getPagesPath();

QString getSettingsPath();

QString getRuntimeSettingsPath();

QString getStoragePath();

QString getMacFilePath();

QString getAutoupdaterPath();

QString getTmpAutoupdaterPath();

QString getProxyConfigPath();

void clearAutoupdatersPath();

void initializeAllPaths();

#endif // PATHS_H
