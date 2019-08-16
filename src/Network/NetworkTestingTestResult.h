#ifndef NETWORKTESTINGTESTRESULT_H
#define NETWORKTESTINGTESTRESULT_H

#include <QString>

struct NetworkTestingTestResult {
    QString host;
    int timeMs;
    bool isTimeout = true;

    NetworkTestingTestResult() = default;

    NetworkTestingTestResult(const QString &host, int timeMs, bool isTimeout)
        : host(host)
        , timeMs(timeMs)
        , isTimeout(isTimeout)
    {}
};

#endif // NETWORKTESTINGTESTRESULT_H
