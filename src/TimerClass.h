#ifndef TIMERCLASS_H
#define TIMERCLASS_H

#include <QObject>

#include <QThread>
#include <QTimer>

#include "duration.h"

// TODO переписать на него все, зависящее от таймера
class TimerClass : public QObject
{
    Q_OBJECT
public:

    TimerClass(const milliseconds &timerPeriod, QObject *parent);

    void exit();

    ~TimerClass();

    void start();

    QThread* getThread();

signals:

    void finished();

    void startedEventInternal();

    void timerEventInternal();

    void finishedEventInternal();

private slots:

    void onStartedEvent();

    void onTimerEvent();

    void onFinishedEvent();

protected:

    virtual void startMethod() = 0;

    virtual void timerMethod() = 0;

    virtual void finishMethod() = 0;

private:

    QThread thread1;

    QTimer qtimer;

private:

    bool isExited = false;

};

#endif // TIMERCLASS_H
