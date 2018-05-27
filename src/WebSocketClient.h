#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QThread>
#include <QtWebSockets/QWebSocket>

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);

    ~WebSocketClient();

    void start();

signals:

    void closed();

    void sendMessage(QString message);

public slots:

    void onStarted();

    void onConnected();
    void onTextMessageReceived(QString message);

    void onSendMessage(QString message);

private:

    QWebSocket m_webSocket;
    QUrl m_url;

    bool isStopped = false;

    QThread thread1;
};

#endif // WEBSOCKETCLIENT_H
