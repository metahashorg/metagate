#include "Settings.hpp"

#include <QCoreApplication>
#include <QSet>
#include <QStandardPaths>
#include <QDir>

QString Settings::configFilePath(const QString &appName) {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+ QDir::separator() + appName + QDir::separator() + Settings::configPathSuffix();
}


Settings::Settings(const QString& path, QObject* parent)
    : QSettings(path, QSettings::IniFormat, parent)
{
}

void Settings::setUpdateType(const updater::UpdateType arg)
{
    QSettings::setValue(updater::updateTypeKey, static_cast<uint>(arg));
}

void Settings::setUpdateFreq(const updater::UpdateFreq arg)
{
    QSettings::setValue(updater::UpdateFreqKey, static_cast<uint>(arg));
}

void Settings::setUpdateTime(const QTime &time)
{
    QSettings::setValue(updater::UpdateTimeKey, time.toString("HH:mm"));
}

void Settings::setUpdateTimeInWeek(const QTime &time)
{
    QSettings::setValue(updater::UpdateTimeInWeekKey, time.toString("HH:mm"));
}

void Settings::setUpdateDays(const QSet<int> &days)
{
    //create space separator string
    QString daysString;
    for (const auto &day : days){
        if (!daysString.isEmpty())
            daysString += " ";
        daysString += QString::number(day);
    }
    QSettings::setValue(updater::UpdateDaysKey, daysString);
}

void Settings::setUpdateNSecs(int secs)
{
    QSettings::setValue(updater::UpdateNSecsKey, secs);
}

void Settings::setUpdateNMins(int mins)
{
    QSettings::setValue(updater::UpdateNMinsKey, mins);
}

updater::UpdateType Settings::updateType() const
{
    const auto &value = QSettings::value(updater::updateTypeKey, static_cast<uint>(updater::updateDefaultType));
    return static_cast<updater::UpdateType>(value.toInt());
}

updater::UpdateFreq Settings::updateFreq() const
{
    const auto &value = QSettings::value(updater::UpdateFreqKey,static_cast<uint>( updater::updateDefaultFreq));
    return static_cast<updater::UpdateFreq>(value.toInt());
}

QTime Settings::updateTime() const
{
    const auto updateTimeStr = QSettings::value(updater::UpdateTimeKey, updater::updateDefaultTime).toString();
    return QTime::fromString(updateTimeStr, "HH:mm");
}

QTime Settings::updateTimeInWeek() const
{
    const auto updateTimeStr = QSettings::value(updater::UpdateTimeInWeekKey, updater::updateDefaultTime).toString();
    return QTime::fromString(updateTimeStr, "HH:mm");
}

QSet<int> Settings::updateDays() const
{
    const auto updateDaysStr = QSettings::value(updater::UpdateDaysKey, updater::updateDefaultDays).toString();
    const auto days = updateDaysStr.split(" ");
    QSet<int> res;
    for (const auto& item : days){
        bool ok = false;
        const auto val = item.toInt(&ok);
        if (ok)
            res.insert(val);
    }
    return res;
}

int Settings::UpdateNSecs() const
{
    return QSettings::value(updater::UpdateNSecsKey, updater::updateDefaultNSecs).toInt();
}

int Settings::UpdateNMins() const
{
    return QSettings::value(updater::UpdateNMinsKey, updater::updateDefaultNMins).toInt();
}


QString Settings::configPathSuffix() {
    return QCoreApplication::applicationName() + Settings::extension();
}

QString Settings::extension()
{
    return ".ini";
}
