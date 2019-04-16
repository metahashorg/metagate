#include "NetwrokTesting.h"

#include <QTime>
#include <QTcpSocket>

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"

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
BEGIN_SLOT_WRAPPER
    testHosts();
END_SLOT_WRAPPER
}

void NetwrokTesting::onTimerEvent()
{
BEGIN_SLOT_WRAPPER
    testHosts();
END_SLOT_WRAPPER
}

void NetwrokTesting::testHosts()
{
    std::vector<std::string> result;
    for (const HostPort &hp : testsHostPort) {
        const std::string r = testHostAndPort(hp.first, hp.second);
        result.emplace_back(r);
    }
    for (const std::string &r: result) {
        LOG << "NetworkTesting: " + r;
    }
}

std::string NetwrokTesting::testHostAndPort(const QString &host, quint16 port)
{
    QTime timer;
    timer.start();
    QTcpSocket socket(this);
    socket.connectToHost(host, port);
    std::string result;
    if (socket.waitForConnected(10000)) {
        result = host.toStdString() + ":" + std::to_string(port) + " connected " + std::to_string(timer.elapsed()) + " ms";
        socket.disconnectFromHost();
    } else {
        result = host.toStdString() + ":" + std::to_string(port) + " not connected 10s timeout";
    }
    return result;
}
