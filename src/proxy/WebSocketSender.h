#ifndef WEBSOCKETSENDER_H
#define WEBSOCKETSENDER_H

#include <QObject>

class WebSocketClient;

struct TypedException;

namespace proxy {

class Proxy;

class WebSocketSender : public QObject
{
    Q_OBJECT
public:

    explicit WebSocketSender(WebSocketClient &client, Proxy &proxyManager, QObject *parent = nullptr);

signals:

    void tryStartTest();

    void testResult(int code, QString message);

private slots:

    void onWssReceived(QString message);

    void onTryStartTest();

    void onModuleFound();

    void onStartAutoProxyResult(const TypedException &r);

    void onStartAutoUPnPResult(const TypedException &r);

    void onStartAutoComplete(quint16 port);

private:

    WebSocketClient &client;

    Proxy &proxyManager;

    QString myIp;

    int port = 0;

    bool startComplete = false;
};

}

#endif // WEBSOCKETSENDER_H
