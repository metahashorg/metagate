#ifndef UNZIP_H
#define UNZIP_H

#include <QString>

#include <string>

void extractDir(QString fileCompressed, QString dir);

void compressDir(QString dir, QString fileCompressed);

void backupKeys(QString dir, QString fileCompressed);

std::string checkBackupFile(QString fileCompressed);

void restoreKeys(QString fileCompressed, QString dir);

#endif // UNZIP_H
