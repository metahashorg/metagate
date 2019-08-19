#ifndef NETWROKTESTING_H
#define NETWROKTESTING_H

#include "qt_utilites/TimerClass.h"
#include "qt_utilites/CallbackWrapper.h"

class NetwrokTesting : public QObject, public TimerClass {
    Q_OBJECT
public:

    struct TestResult {
        QString host;
        int timeMs;
        bool isTimeout = true;

        TestResult() = default;

        TestResult(const QString &host, int timeMs, bool isTimeout)
            : host(host)
            , timeMs(timeMs)
            , isTimeout(isTimeout)
        {}
    };

public:

    using GetTestResultsCallback = CallbackWrapper<void(const std::vector<TestResult> &results)>;

public:
    explicit NetwrokTesting(QObject *parent = nullptr);

    ~NetwrokTesting() override;

signals:

    void getTestResults(const GetTestResultsCallback &callback);

public slots:

    void onGetTestResults(const GetTestResultsCallback &callback);

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

private:
    void testHosts();
    TestResult testHostAndPort(const QString &host, quint16 port);

private:

    std::vector<TestResult> lastResults;
};

#endif // NETWROKTESTING_H
