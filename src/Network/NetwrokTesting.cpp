#include "NetwrokTesting.h"

#include <QTime>

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

    Q_CONNECT(&client, &HttpSimpleClient::callbackCall, this, &NetwrokTesting::callbackCall);

    client.moveToThread(TimerClass::getThread());
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

void NetwrokTesting::processTest() {
    if (index >= testsHostPort.size()) {
        index = 0;

        const auto resultToString = [](const NetworkTestingTestResult &r) {
            if (r.isTimeout) {
                return r.host.toStdString() + " not connected 10s timeout";
            } else {
                return r.host.toStdString() + " connected " + std::to_string(r.timeMs) + " ms";
            }
        };

        for (const NetworkTestingTestResult &r: lastResults) {
            LOG << "NetworkTesting: " + resultToString(r);
        }

        return;
    }

    const HostPort &hp = testsHostPort[index];
    const static seconds timeout = 10s;
    const time_point startTime = ::now();
    client.sendMessagePing("http://" + hp.first + ":" + QString::number(hp.second), HttpSimpleClient::ClientCallback([this, startTime, address=hp.first](const std::string &response, const TypedException &e) {
        const time_point stopTime = ::now();
        const milliseconds time = std::chrono::duration_cast<milliseconds>(stopTime - startTime);
        NetworkTestingTestResult res(address, time.count(), time >= timeout);

        lastResults.emplace_back(res);

        index++;

        processTest();
    }), timeout);
}

void NetwrokTesting::testHosts() {
    lastResults.clear();
    processTest();
}

void NetwrokTesting::onGetTestResults(const GetTestResultsCallback &callback) {
BEGIN_SLOT_WRAPPER
    callback.emitFunc(TypedException(), lastResults);
END_SLOT_WRAPPER
}
