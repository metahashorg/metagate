#ifndef UTILS_H
#define UTILS_H

#include <string>

#include <QString>

std::string toHex(const std::string &data);

std::string toBase64(const std::string &value);

std::string base58ToHex(const std::string &value);

std::string fromHex(const std::string &value);

bool isDecimal(const std::string &str);

void writeToFile(const QString &pathToFile, const std::string &data, bool isCheck);

void writeToFileBinary(const QString &pathToFile, const std::string &data, bool isCheck);

std::string readFile(const QString &pathToFile);

bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath, bool isDirs = true);

#endif // UTILS_H
