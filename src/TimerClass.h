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
    explicit TimerClass(const milliseconds &timerPeriod, QObject *parent = nullptr);

    ~TimerClass();

    void start();

signals:

    void finished();

    void startedEvent();

    void timerEvent();

public slots:

protected:

    QThread thread1;

    QTimer qtimer;

};

#endif // TIMERCLASS_H
