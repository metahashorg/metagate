#ifndef WEBSOCKETSENDER_H
#define WEBSOCKETSENDER_H

#include <QObject>

class WebSocketClient;

namespace proxy {

class WebSocketSender : public QObject
{
    Q_OBJECT
public:

    explicit WebSocketSender(WebSocketClient &client, QObject *parent = nullptr);

signals:

    void startTest();

    void testResult(int code, QString message);

private slots:

    void onWssReceived(QString message);

    void onStartTest();

private:

    WebSocketClient &client;

    QString myIp;

    int port = 0;
};

}

#endif // WEBSOCKETSENDER_H
