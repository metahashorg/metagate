#ifndef INITINTERFACE_H
#define INITINTERFACE_H

#include "duration.h"

#include <functional>

#include <string>
#include <vector>
#include <mutex>

#include <QTimer>
#include <QObject>

class QThread;

struct TypedException;

namespace initializer {

class Initializer;
struct InitState;

class InitInterface: public QObject {
    Q_OBJECT
public:

    InitInterface(const QString &type, QThread *mainThread, Initializer &manager, bool isTimerEnabled);

    virtual ~InitInterface() = default;

    void sendState(const InitState &state);

    virtual void complete() = 0;

protected:

    // Вызывать только из конструктора, не из стороннего треда
    void setTimerEvent(const milliseconds &timer, const std::string &errorDesc, const std::function<void(const TypedException &e)> &func);

private slots:

    void onTimerEvent();

protected:

    QThread *mainThread;

    Initializer &manager;

private:

    const bool isTimerEnabled;

    const QString type;

    QTimer timer;

    std::vector<std::tuple<time_point, milliseconds, std::string, std::function<void(const TypedException &e)>>> events;
    std::mutex eventsMut;

};

}

#endif // INITINTERFACE_H
