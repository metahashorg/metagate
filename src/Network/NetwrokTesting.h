#ifndef NETWROKTESTING_H
#define NETWROKTESTING_H

#include "qt_utilites/TimerClass.h"

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "NetworkTestingTestResult.h"

#include "HttpClient.h"

class NetwrokTesting : public ManagerWrapper, public TimerClass {
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

    void processTest();

private:

    size_t index = 0;

    std::vector<NetworkTestingTestResult> lastResults;

    HttpSimpleClient client;
};

#endif // NETWROKTESTING_H
