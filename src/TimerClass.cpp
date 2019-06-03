#include "TimerClass.h"

#include "check.h"
#include "Log.h"

TimerClass::TimerClass(const milliseconds &timerPeriod, QObject *parent)
    : QObject(parent)
{
    CHECK(connect(&thread1, &QThread::started, this, &TimerClass::startedEvent), "not connect startedEvent");
    CHECK(connect(this, &TimerClass::finished, &thread1, &QThread::terminate), "not connect terminate");

    qtimer.moveToThread(&thread1);
    qtimer.setInterval(timerPeriod.count());
    CHECK(connect(&qtimer, &QTimer::timeout, this, &TimerClass::timerEvent), "not connect timerEvent");
    CHECK(connect(&thread1, &QThread::finished, this, &TimerClass::finishedEvent), "not connect finishedEvent");
    CHECK(connect(&thread1, &QThread::started, &qtimer, QOverload<>::of(&QTimer::start)), "not connect start");
    CHECK(connect(&thread1, &QThread::finished, &qtimer, &QTimer::stop), "not connect stop");
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
