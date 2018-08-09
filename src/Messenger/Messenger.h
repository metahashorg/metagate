#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObject>

#include "TimerClass.h"

class Messenger : public TimerClass
{
    Q_OBJECT
public:
    explicit Messenger(QObject *parent = nullptr);

signals:

public slots:

    void onTimerEvent();

};

#endif // MESSENGER_H
