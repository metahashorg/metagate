#include "unzip.h"

#include <iostream>

#include <QDateTime>
#include <QStandardPaths>

#include "check.h"
#include "utils.h"

#include <quazip/JlCompress.h>

void extractDir(QString fileCompressed, QString dir) {
    const QStringList lst = JlCompress::extractDir(fileCompressed, dir);
    CHECK(!lst.empty(), "Error extracted archive " + fileCompressed.toStdString());
}

void compressDir(QString dir, QString fileCompressed) {
    const bool rslt = JlCompress::compressDir(fileCompressed, dir, true);
    CHECK(rslt, "Error while compressed archive " + fileCompressed.toStdString());
}

void backupKeys(QString dir, QString fileCompressed) {
    const QString backupFileIdent = makePath(dir, "meta.info");
    const QDateTime time = QDateTime::currentDateTime();
    writeToFile(backupFileIdent, time.toString().toStdString(), false);
    compressDir(dir, fileCompressed);
    QDir().remove(backupFileIdent);
}

std::string checkBackupFile(QString fileCompressed) {
    const QString metaFile = makePath(QStandardPaths::writableLocation(QStandardPaths::TempLocation), "meta.info");
    const QString tempFile = JlCompress::extractFile(fileCompressed, "meta.info", metaFile);
    CHECK(!tempFile.isNull() && !tempFile.isEmpty(), "Not supported backup file format");
    const std::string content = readFile(tempFile);
    return content;
}

void restoreKeys(QString fileCompressed, QString dir) {
    extractDir(fileCompressed, dir);
    const QString backupFileIdent = makePath(dir, "meta.info");
    QDir().remove(backupFileIdent);
}
