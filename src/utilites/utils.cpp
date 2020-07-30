#include "utils.h"

#include <vector>

#include <QString>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>

#include "check.h"

std::string toHex(const std::string &data) {
    QByteArray array(data.data(), data.size());
    return QString(array.toHex()).toStdString();
}

QString toHex(const QString &data) {
    QByteArray array = data.toUtf8();
    return QString(array.toHex());
}

std::string toBase64(const std::string &value) {
    QByteArray array(value.data(), value.size());
    return QString(array.toBase64()).toStdString();
}

QString toBase64(const QString &value) {
    QByteArray array(value.toUtf8());
    return QString(array.toBase64());
}

std::string fromBase64(const std::string &value) {
    QByteArray pubKeyArray(value.data(), value.size());
    return QByteArray::fromBase64(pubKeyArray).toStdString();
}

QString fromBase64(const QString &value) {
    QByteArray pubKeyArray(value.toUtf8());
    return QByteArray::fromBase64(pubKeyArray);
}

std::string fromHex(const std::string &value) {
    QByteArray pubKeyArray(value.data(), value.size());
    return QByteArray::fromHex(pubKeyArray).toStdString();
}

QString fromHex(const QString &data) {
    QByteArray pubKeyArray = data.toUtf8();
    return QString(QByteArray::fromHex(pubKeyArray));
}

bool isDecimal(const std::string &str) {
    if (str.size() >= 2) {
        if (str.substr(0, 2) == "0x") {
            return false;
        }
    }
    for (const char c: str) {
        if ('0' <= c && c <= '9') {
            // ok
        } else {
            return false;
        }
    }
    return true;
}

bool isHex(const std::string &str) {
    if (str.size() < 2) {
        return false;
    }
    if (str.substr(0, 2) != "0x") {
        return false;
    }
    for (const char c: str.substr(2)) {
        if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
            // ok
        } else {
            return false;
        }
    }
    return true;
}

std::string toLower(const std::string &str) {
    return QString::fromStdString(str).toLower().toStdString();
}

std::string trim(const std::string &str) {
    return QString::fromStdString(str).trimmed().toStdString();
}

void writeToFile(const QString &pathToFile, const std::string &data, bool isCheck) {
    QFile file(pathToFile);
    if (isCheck) {
        CHECK(!file.exists(), "File " + pathToFile.toStdString() + " is exist");
    }
    CHECK(file.open(QIODevice::WriteOnly), "File not open " + pathToFile.toStdString());
    QTextStream in(&file);
    in << QString::fromStdString(data);
    in.flush();
    file.close();
}

void writeToFileBinary(const QString &pathToFile, const QByteArray &data, bool isCheck) {
    QFile file(pathToFile);
    if (isCheck) {
        CHECK(!file.exists(), "File " + pathToFile.toStdString() + " is exist");
    }
    CHECK(file.open(QIODevice::WriteOnly), "File not open " + pathToFile.toStdString());
    file.write(data);
    file.close();
}

std::string readFile(const QString &pathToFile) {
    QFile file(pathToFile);
    CHECK(file.open(QIODevice::ReadOnly), "File not open " + pathToFile.toStdString());
    QTextStream in(&file);
    QString str = in.readAll();
    return str.toStdString();
}

std::string readFileBinary(const QString &pathToFile) {
    QFile file(pathToFile);
    CHECK(file.open(QIODevice::ReadOnly), "File not open " + pathToFile.toStdString());
    return file.readAll().toStdString();
}

bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath, bool isDirs, bool isReplace) {
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        targetDir.mkpath(tgtFilePath);
        QDir sourceDir(srcFilePath);
        auto mask = QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System;
        if (isDirs) {
            mask |= QDir::Dirs;
        }
        QStringList fileNames = sourceDir.entryList(mask);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                    = QDir(srcFilePath).filePath(fileName);
            const QString newTgtFilePath
                    = QDir(tgtFilePath).filePath(fileName);
            if (!copyRecursively(newSrcFilePath, newTgtFilePath, isDirs, isReplace)) {
                return false;
            }
        }
    } else {
        if (isReplace) {
            QFile::remove(tgtFilePath);
        }
        if (!QFile::copy(srcFilePath, tgtFilePath)) {
            return false;
        }
    }
    return true;
}

void copyFile(const QString &srcFilePath, const QString &tgtFilePath, bool isReplace) {
    if (isReplace) {
        QFile::remove(tgtFilePath);
    }
    CHECK(QFile::copy(srcFilePath, tgtFilePath), "Dont copy file " + tgtFilePath.toStdString());
}

void copyToDirectoryFile(const QString &srcFilePath, const QString &tgtDirectory, bool isReplace) {
    createFolder(tgtDirectory);
    const QString fileName = QFileInfo(srcFilePath).fileName();
    const QString newFile = makePath(tgtDirectory, fileName);
    copyFile(srcFilePath, newFile, isReplace);
}

void createFolder(const QString &folder) {
    QDir dir(folder);
    const bool resultCreate = dir.mkpath(folder);
    CHECK(resultCreate, "dont create folder " + folder.toStdString());
}

bool isExistFile(const QString &file) {
    return QFile::exists(file);
}

bool isExistFolder(const QString &folder) {
    QDir dir(folder);
    return dir.exists();
}

void removeFile(const QString &file) {
    QFile::remove(file);
}

void removeFolder(const QString &folder) {
    QDir dir(folder);
    dir.removeRecursively();
}

bool isPathEquals(const QString &path1, const QString &path2) {
    QString p1(path1);
    QString p2(path2);

    if (p1.endsWith("/") || p1.endsWith("\\")) {
        p1.remove(p1.size() - 1, 1);
    }
    if (p2.endsWith("/") || p2.endsWith("\\")) {
        p2.remove(p1.size() - 1, 1);
    }

    return p1 == p2;
}

QString getFileName(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.fileName();
}

QString getBaseName(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.baseName();
}

QString getExtension(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.completeSuffix();
}

bool isDirectory(const QString &path) {
    QFileInfo fileInfo(path);
    return fileInfo.isDir();
}

QStringList getFilesForDir(const QString &path) {
    const QDir dir(path);
    return dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
}

bool isPathInDir(const QString &path, const QString &dirpath)
{
    QDir dir(path);
    const QDir odir(dirpath);
    while (dir != odir) {
        if (!dir.cdUp())
            return false;
    }
    return true;
}
