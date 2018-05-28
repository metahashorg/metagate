#include "utils.h"

#include <vector>

#include <QString>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>

#include "btctx/Base58.h"

#include "check.h"

std::string toHex(const std::string &data) {
    QByteArray array(data.data(), data.size());
    return QString(array.toHex()).toStdString();
}

std::string toBase64(const std::string &value) {
    QByteArray array(value.data(), value.size());
    return QString(array.toBase64()).toStdString();
}

std::string base58ToHex(const std::string &value) {
    std::vector<unsigned char> decoded;
    DecodeBase58(value.data(), decoded);
    std::string decodedStr(decoded.begin(), decoded.end());
    return toHex(decodedStr);
}

std::string fromHex(const std::string &value) {
    QByteArray pubKeyArray(value.data(), value.size());
    return QByteArray::fromHex(pubKeyArray).toStdString();
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

void writeToFileBinary(const QString &pathToFile, const std::string &data, bool isCheck) {
    QFile file(pathToFile);
    if (isCheck) {
        CHECK(!file.exists(), "File " + pathToFile.toStdString() + " is exist");
    }
    CHECK(file.open(QIODevice::WriteOnly), "File not open " + pathToFile.toStdString());
    const QByteArray byteArray(data.data(), data.size());
    file.write(byteArray);
    file.close();
}

std::string readFile(const QString &pathToFile) {
    QFile file(pathToFile);
    CHECK(file.open(QIODevice::ReadOnly), "File not open " + pathToFile.toStdString());
    QTextStream in(&file);
    QString str = in.readAll();
    return str.toStdString();
}

bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath, bool isDirs) {
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
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath))
            return false;
    }
    return true;
}

void createFolder(const QString &folder) {
    QDir dir(folder);
    const bool resultCreate = dir.mkpath(folder);
    CHECK(resultCreate, "dont create folder " + folder.toStdString());
}
