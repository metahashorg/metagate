#include "TimerClass.h"

#include "check.h"

TimerClass::TimerClass(const milliseconds &timerPeriod, QObject *parent)
    : QObject(parent)
{
    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(startedEvent())), "not connect");
    CHECK(QObject::connect(this,SIGNAL(finished()),&thread1,SLOT(terminate())), "not connect");

    qtimer.moveToThread(&thread1);
    qtimer.setInterval(timerPeriod.count());
    CHECK(connect(&qtimer, SIGNAL(timeout()), this, SLOT(timerEvent())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(started()), SLOT(start())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(finished()), SLOT(stop())), "not connect");
}

TimerClass::~TimerClass() {
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }
}

void TimerClass::start() {
    thread1.start();
}
