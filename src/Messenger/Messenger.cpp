#include "Messenger.h"

#include "check.h"

Messenger::Messenger(QObject *parent)
    : TimerClass(1s, parent)
{
    CHECK(connect(this, SIGNAL(timerEvent()), this, SLOT(onTimerEvent())), "not connect");
}

void Messenger::onTimerEvent() {

}
