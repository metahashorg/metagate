#ifndef NETWROKTESTING_H
#define NETWROKTESTING_H

#include "qt_utilites/TimerClass.h"
#include "qt_utilites/CallbackWrapper.h"

#include "NetworkTestingTestResult.h"

class NetwrokTesting : public QObject, public TimerClass {
    Q_OBJECT
public:

    using GetTestResultsCallback = CallbackWrapper<void(const std::vector<NetworkTestingTestResult> &results)>;

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
    NetworkTestingTestResult testHostAndPort(const QString &host, quint16 port);

private:

    std::vector<NetworkTestingTestResult> lastResults;
};

#endif // NETWROKTESTING_H
