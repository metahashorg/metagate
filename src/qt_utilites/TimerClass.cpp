#include "TimerClass.h"

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"
#include "QRegister.h"

TimerClassImpl::TimerClassImpl(TimerClass *tc, const milliseconds &timerPeriod, QObject *parent)
    : QObject(parent)
    , tc(tc)
{
    Q_CONNECT(&thread1, &QThread::started, this, &TimerClassImpl::onStartedEvent);

    qtimer.moveToThread(&thread1);
    qtimer.setInterval(timerPeriod.count());
    Q_CONNECT(&qtimer, &QTimer::timeout, this, &TimerClassImpl::onTimerEvent);
    Q_CONNECT(&thread1, &QThread::finished, this, &TimerClassImpl::onFinishedEvent);
    Q_CONNECT(&thread1, &QThread::started, &qtimer, QOverload<>::of(&QTimer::start));
    Q_CONNECT(&thread1, &QThread::finished, &qtimer, &QTimer::stop);
    moveToThread(&thread1);
}

void TimerClassImpl::exit() {
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }

    isExited = true;
}

TimerClassImpl::~TimerClassImpl() {
    if (!isExited) {
        LOG << "Error: child class not call TimerClass::exit method in destructor";
    }
}

void TimerClassImpl::start() {
    thread1.start();
}

QThread* TimerClassImpl::getThread() {
    return &thread1;
}

void TimerClassImpl::onStartedEvent() {
BEGIN_SLOT_WRAPPER
    tc->startMethod();
END_SLOT_WRAPPER
}

void TimerClassImpl::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    tc->timerMethod();
END_SLOT_WRAPPER
}

void TimerClassImpl::onFinishedEvent() {
BEGIN_SLOT_WRAPPER
    tc->finishMethod();
END_SLOT_WRAPPER
}

TimerClass::TimerClass(const milliseconds &timerPeriod, QObject *parent)
    : impl(this, timerPeriod, parent)
{}

void TimerClass::exit() noexcept {
    try {
        impl.exit();
    } catch (...) {
        LOG << "Error while exit TimerClass";
    }
}

TimerClass::~TimerClass() = default;

void TimerClass::start() {
    impl.start();
}

QThread* TimerClass::getThread() {
    return impl.getThread();
}
