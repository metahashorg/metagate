#ifndef NETWROKTESTING_H
#define NETWROKTESTING_H

#include "TimerClass.h"

class NetwrokTesting : public TimerClass
{
    Q_OBJECT
public:
    explicit NetwrokTesting(QObject *parent = nullptr);

private slots:
    void onStarted();

    void onTimerEvent();


private:
    void testHosts();
    std::string testHostAndPort(const QString &host, quint16 port);
};

#endif // NETWROKTESTING_H
