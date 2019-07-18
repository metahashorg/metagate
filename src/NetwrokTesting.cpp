#include "NetwrokTesting.h"

#include <QTime>
#include <QTcpSocket>

#include "check.h"
#include "Log.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

using HostPort = QPair<QString, quint16>;
static const QList<HostPort> testsHostPort {
                {QStringLiteral("www.google.com"), 80},
                {QStringLiteral("1.1.1.1"), 80},
                {QStringLiteral("echo.metahash.io"), 7654}
};

NetwrokTesting::NetwrokTesting(QObject *parent)
    : TimerClass(5min, parent)
{
    Q_CONNECT(this, &NetwrokTesting::getTestResults, this, &NetwrokTesting::onGetTestResults);

    Q_REG(GetTestResultsCallback, "GetTestResultsCallback");

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

void NetwrokTesting::testHosts() {
    lastResults.clear();
    for (const HostPort &hp : testsHostPort) {
        const TestResult r = testHostAndPort(hp.first, hp.second);
        lastResults.emplace_back(r);
    }

    const auto resultToString = [](const TestResult &r) {
        if (r.isTimeout) {
            return r.host.toStdString() + " not connected 10s timeout";
        } else {
            return r.host.toStdString() + " connected " + std::to_string(r.timeMs) + " ms";
        }
    };

    for (const TestResult &r: lastResults) {
        LOG << "NetworkTesting: " + resultToString(r);
    }
}

NetwrokTesting::TestResult NetwrokTesting::testHostAndPort(const QString &host, quint16 port) {
    QTime timer;
    timer.start();
    QTcpSocket socket(this);
    socket.connectToHost(host, port);
    TestResult result;
    if (socket.waitForConnected(10000)) {
        result = TestResult(host + ":" + QString::fromStdString(std::to_string(port)), timer.elapsed(), false);
        socket.disconnectFromHost();
    } else {
        result = TestResult(host + ":" + QString::fromStdString(std::to_string(port)), 0, true);
    }
    return result;
}

void NetwrokTesting::onGetTestResults(const GetTestResultsCallback &callback) {
BEGIN_SLOT_WRAPPER
    callback.emitFunc(TypedException(), lastResults);
END_SLOT_WRAPPER
}
