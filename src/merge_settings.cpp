#include "merge_settings.h"

#include <QSettings>

#include <map>
#include <string>
#include <vector>

#include "VersionWrapper.h"
#include "utils.h"

static std::vector<std::string> splitStr(const std::string &str, const std::string &delimiter) {
    std::vector<std::string> result;
    size_t foundPos = 0;
    while (true) {
        const size_t newPos = str.find(delimiter, foundPos);
        result.emplace_back(str.substr(foundPos, newPos - foundPos));
        if (newPos == str.npos) {
            break;
        }
        foundPos = newPos + delimiter.size();
    }
    return result;
}

static std::map<std::string, VersionVar> parseSpecFile(const QString &specFile) {
    std::map<std::string, VersionVar> result;

    const std::string fileContent = readFile(specFile);
    const std::vector<std::string> lines = splitStr(fileContent, "\n");
    for (const std::string &line: lines) {
        const std::string stripLine = trim(line);
        if (stripLine.empty()) {
            continue;
        }
        if (stripLine.find("//") == 0) {
            continue;
        }
        const size_t foundDots = stripLine.find(":");
        const VersionVar version(stripLine.substr(0, foundDots));
        const std::vector<std::string> strings = splitStr(stripLine.substr(foundDots + 1), " ");
        for (const std::string &string: strings) {
            const std::string splitStr = trim(string);
            if (splitStr.empty()) {
                continue;
            }
            result[splitStr] = std::max(result[splitStr], version);
        }
    }

    return result;
}

static QString makeKey(const QString &first, const QString &second) {
    if (first.isEmpty()) {
        return second;
    } else {
        return first + "/" + second;
    }
}

static void copyMerge(QSettings &oldSettings, QSettings &newSettings, const QString &currentGroup) {
    oldSettings.remove(currentGroup);
    const QStringList keys = newSettings.childKeys();
    for (const QString &key: keys) {
        oldSettings.setValue(makeKey(currentGroup, key), newSettings.value(key));
    }

    const QStringList groups = newSettings.childGroups();
    for (const QString &key: groups) {
        newSettings.beginGroup(makeKey(currentGroup, key)); // TODO не факт, что этот код идейно правильный, но так как нельзя создавать рекурсивные группы, то пофиг
        copyMerge(oldSettings, newSettings, makeKey(currentGroup, key));
        newSettings.endGroup();
    }

    const QString saveGroup = newSettings.group();
    newSettings.endGroup();
    const int sizeArray = newSettings.beginReadArray(currentGroup);
    if (sizeArray != 0) {
        oldSettings.beginWriteArray(currentGroup);
        for (int i = 0; i < sizeArray; i++) {
            newSettings.setArrayIndex(i);
            oldSettings.setArrayIndex(i);

            const QStringList keysArray = newSettings.childKeys();
            for (const QString &key: keysArray) {
                oldSettings.setValue(key, newSettings.value(key));
            }
        }
        oldSettings.endArray();
    }
    newSettings.endArray();
    newSettings.beginGroup(saveGroup);
}

static void recMerge(QSettings &oldSettings, QSettings &newSettings, const VersionVar &oldSettingsVersion, const std::map<std::string, VersionVar> &specs, const QString &currentGroup) {
    const auto isChangeGroup = [&oldSettingsVersion, &specs](const QString &name) {
        return specs.find(name.toStdString()) != specs.end() && oldSettingsVersion < specs.at(name.toStdString());
    };

    const auto isChangeKey = [&oldSettings, &oldSettingsVersion, &specs](const QString &name) {
        return !name.isEmpty() && !oldSettings.contains(name) || (specs.find(name.toStdString()) != specs.end() && oldSettingsVersion < specs.at(name.toStdString()));
    };

    if (isChangeGroup(currentGroup)) {
        copyMerge(oldSettings, newSettings, currentGroup);
        return;
    }

    const QStringList keys = newSettings.childKeys();
    for (const QString &key: keys) {
        const QString currentSetting = makeKey(currentGroup, key);
        if (isChangeKey(currentSetting)) {
            oldSettings.setValue(currentSetting, newSettings.value(key));
        }
    }

    const QStringList groups = newSettings.childGroups();
    for (const QString &key: groups) {
        newSettings.beginGroup(makeKey(currentGroup, key));
        recMerge(oldSettings, newSettings, oldSettingsVersion, specs, makeKey(currentGroup, key));
        newSettings.endGroup();
    }
}

void mergeSettings(const QString &oldSettingsPath, const QString &newSettingsPath, const QString &specFile) {
    const std::map<std::string, VersionVar> specs = parseSpecFile(specFile);
    QSettings oldSettings(oldSettingsPath, QSettings::IniFormat);
    QSettings newSettings(newSettingsPath, QSettings::IniFormat);
    const VersionVar oldSettingsVersion(oldSettings.value("version").toString().toStdString());

    recMerge(oldSettings, newSettings, oldSettingsVersion, specs, newSettings.group());

    oldSettings.setValue("version", newSettings.value("version"));

    oldSettings.sync();
}
