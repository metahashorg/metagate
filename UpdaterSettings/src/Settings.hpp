#include <../AutoUpdaterService/CppWindowsService/Schedule.hpp>

#include <QSettings>
#include <QTime>

class Settings : public QSettings {
public:
    Settings(const QString& path, QObject* parent = nullptr);

    //
    void setUpdateType(const updater::UpdateType arg);
    void setUpdateFreq(const updater::UpdateFreq arg);
    void setUpdateTime(const QTime &time);
    void setUpdateTimeInWeek(const QTime &time);
    void setUpdateDays(const QSet<int> &days);
    void setUpdateNSecs(int secs);
    void setUpdateNMins(int mins);

    updater::UpdateType updateType() const;
    updater::UpdateFreq updateFreq() const;
    QTime updateTime() const;
    QTime updateTimeInWeek() const;
    QSet<int> updateDays() const;
    int UpdateNSecs() const;
    int UpdateNMins() const;


    const char *UpdateTimeKey = "UpdateTime";
    const char *UpdateDaysKey = "UpdateDays";

    static QString configPathSuffix();
    static QString  configFilePath(const QString &appName);
    static QString extension();
};
