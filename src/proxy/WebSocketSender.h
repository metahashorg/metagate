#ifndef WEBSOCKETSENDER_H
#define WEBSOCKETSENDER_H

#include <QObject>

#include "Network/WebSocketClient.h"

struct TypedException;

namespace proxy {

class Proxy;

class WebSocketSender : public QObject
{
    Q_OBJECT
public:

    explicit WebSocketSender(Proxy &proxyManager, QObject *parent = nullptr);

signals:

    void tryStartTest();

    void testResult(int code, QString message);

    void proxyTested(bool res, const QString &error);

private slots:

    void onWssReceived(QString message);

    void onTryStartTest();

    void onBeginStart();

    void onStartAutoProxyResult(const TypedException &r);

    void onStartAutoUPnPResult(const TypedException &r);

    void onReadyToTest(quint16 port);

    void onTestResult(int code, QString result);

    void onStopProxy();

private:

    WebSocketClient client;

    Proxy &proxyManager;

    QString myIp;

    int port = 0;

    bool startComplete = false;
};

}

#endif // WEBSOCKETSENDER_H
