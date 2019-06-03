#ifndef TIMERCLASS_H
#define TIMERCLASS_H

#include <QObject>

#include <QThread>
#include <QTimer>

#include "duration.h"

class TimerClass : public QObject {
    Q_OBJECT
public:

    TimerClass(const milliseconds &timerPeriod, QObject *parent);

    void exit();

    virtual ~TimerClass();

    void start();

    QThread* getThread();

signals:

    void finished();

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

    bool isExited = false;

};

#endif // TIMERCLASS_H
