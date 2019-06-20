#include "TimerClass.h"

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"
#include "QRegister.h"

TimerClass::TimerClass(const milliseconds &timerPeriod, QObject *parent)
    : QObject(parent)
{
    Q_CONNECT(&thread1, &QThread::started, this, &TimerClass::onStartedEvent);
    Q_CONNECT(this, &TimerClass::finished, &thread1, &QThread::terminate);

    qtimer.moveToThread(&thread1);
    qtimer.setInterval(timerPeriod.count());
    Q_CONNECT(&qtimer, &QTimer::timeout, this, &TimerClass::onTimerEvent);
    Q_CONNECT(&thread1, &QThread::finished, this, &TimerClass::onFinishedEvent);
    Q_CONNECT(&thread1, &QThread::started, &qtimer, QOverload<>::of(&QTimer::start));
    Q_CONNECT(&thread1, &QThread::finished, &qtimer, &QTimer::stop);
}

void TimerClass::exit() {
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }

    isExited = true;
}

TimerClass::~TimerClass() {
    if (!isExited) {
        LOG << "Error: child class not call TimerClass::exit method in destructor";
    }
}

void TimerClass::start() {
    thread1.start();
}

QThread* TimerClass::getThread() {
    return &thread1;
}

void TimerClass::onStartedEvent() {
BEGIN_SLOT_WRAPPER
    startMethod();
END_SLOT_WRAPPER
}

void TimerClass::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    timerMethod();
END_SLOT_WRAPPER
}

void TimerClass::onFinishedEvent() {
BEGIN_SLOT_WRAPPER
    finishMethod();
END_SLOT_WRAPPER
}
