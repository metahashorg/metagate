#ifndef TIMERCLASS_H
#define TIMERCLASS_H

#include <QObject>

#include <QThread>
#include <QTimer>

#include "duration.h"

class TimerClass;

class TimerClassImpl : public QObject {
    Q_OBJECT
public:

    TimerClassImpl(TimerClass *tc, const milliseconds &timerPeriod, QObject *parent);

    void exit();

    ~TimerClassImpl();

    void start();

    QThread* getThread();

private slots:

    void onStartedEvent();

    void onTimerEvent();

    void onFinishedEvent();

private:

    TimerClass *tc;

    QThread thread1;

    QTimer qtimer;

    bool isExited = false;

};

class TimerClass {
    friend class TimerClassImpl;
public:

    TimerClass(const milliseconds &timerPeriod, QObject *parent);

    void exit();

    virtual ~TimerClass();

    void start();

    QThread* getThread();

protected:

    virtual void startMethod() = 0;

    virtual void timerMethod() = 0;

    virtual void finishMethod() = 0;

private:

    TimerClassImpl impl;

};

#endif // TIMERCLASS_H
