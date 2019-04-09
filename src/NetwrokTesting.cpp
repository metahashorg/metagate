#include "NetwrokTesting.h"

#include <QTime>
#include <QTcpSocket>

#include "check.h"
#include "Log.h"

using HostPort = QPair<QString, quint16>;
static const QList<HostPort> testsHostPort {
                {QStringLiteral("www.google.com"), 80},
                {QStringLiteral("74.125.232.243"), 80}
};

NetwrokTesting::NetwrokTesting(QObject *parent)
    : TimerClass(5min, parent)
{

    CHECK(connect(this, &NetwrokTesting::timerEvent, this, &NetwrokTesting::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &NetwrokTesting::startedEvent, this, &NetwrokTesting::onStarted), "not connect onStarted");

    moveToThread(&thread1); // TODO вызывать в TimerClass
}

void NetwrokTesting::onStarted()
{
    testHosts();
}

void NetwrokTesting::onTimerEvent()
{
    testHosts();
}

void NetwrokTesting::testHosts()
{
    for (const HostPort &hp : testsHostPort)
        testHostAndPort(hp.first, hp.second);
}

void NetwrokTesting::testHostAndPort(const QString &host, quint16 port)
{
    QTime timer;
    timer.start();
    QTcpSocket socket(this);
    socket.connectToHost(host, port);
    if (socket.waitForConnected(10000)) {
        LOG << "NetworkTesting: " << host << ":" << port << " connected " << timer.elapsed() << " ms";
        socket.disconnectFromHost();
        socket.waitForDisconnected();
    } else {
        LOG << "NetworkTesting: " << host << ":" << port << " not connected 10s timeout";
    }
}
