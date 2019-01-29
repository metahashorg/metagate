#ifndef INITINTERFACE_H
#define INITINTERFACE_H

#include "duration.h"

#include <functional>

#include <string>
#include <vector>
#include <mutex>
#include <map>

#include <QTimer>
#include <QObject>

class QThread;

struct TypedException;

namespace initializer {

class Initializer;
struct InitState;

class InitInterface: public QObject {
    Q_OBJECT
private:

    struct StateType {
        const bool isCritical;
        const QString message;
        const bool isTimeout;
        const bool isOneRun;

        bool isSended = false;
        bool isTimeoutSended = false;
        bool isSuccess = false;

        StateType(bool isCritical, const QString &message, bool isTimeout, bool isOneRun);
    };

public:

    InitInterface(const QString &type, QThread *mainThread, Initializer &manager, bool isTimerEnabled);

    virtual ~InitInterface() = default;

    void complete();

    QString getType() const;

    std::vector<std::tuple<QString, QString, bool>> getSubtypes() const;

protected:

    virtual void completeImpl() = 0;

    void registerStateType(const QString &subType, const QString &message, bool isCritical, bool isOneRun);

    void registerStateType(const QString &subType, const QString &message, bool isCritical, bool isOneRun, const milliseconds &timer, const std::string &errorDesc);

    void sendState(const QString &subType, bool isScipped, const TypedException &exception);

private:

    void registerStateType(const QString &subType, const QString &message, bool isCritical, bool isOneRun, bool isTimeout, const milliseconds &timer, const std::string &errorDesc);

    void setTimerEvent(const milliseconds &timer, const std::string &errorDesc, const std::function<void(const TypedException &e)> &func);

    void sendStateTimeout(const QString &subType, const TypedException &exception);

private slots:

    void onTimerEvent();

protected:

    QThread *mainThread;

    Initializer &manager;

private:

    const QString type;

    const bool isTimerEnabled;

    QTimer timer;

    std::vector<std::tuple<time_point, milliseconds, std::string, std::function<void(const TypedException &e)>>> events;
    std::mutex eventsMut;

    std::map<QString, StateType> states;

    bool isCompleted = false;

};

}

#endif // INITINTERFACE_H
