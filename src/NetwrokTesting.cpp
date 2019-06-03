#include "NetwrokTesting.h"

#include <QTime>
#include <QTcpSocket>

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"

using HostPort = QPair<QString, quint16>;
static const QList<HostPort> testsHostPort {
                {QStringLiteral("www.google.com"), 80},
                {QStringLiteral("74.125.232.243"), 80},
                {QStringLiteral("echo.metahash.io"), 7654}
};

NetwrokTesting::NetwrokTesting(QObject *parent)
    : TimerClass(5min, parent)
{
    moveToThread(TimerClass::getThread());
}

NetwrokTesting::~NetwrokTesting() {
    TimerClass::exit();
}

void NetwrokTesting::startMethod() {
    testHosts();
}

void NetwrokTesting::timerMethod() {
    testHosts();
}

void NetwrokTesting::finishMethod() {
    // empty
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
