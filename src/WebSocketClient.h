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

    void setHelloString(QString message);

signals:

    void messageReceived(QString message);

public slots:

    void onStarted();

    void onConnected();
    void onTextMessageReceived(QString message);

    void onSendMessage(QString message);

    void onSetHelloString(QString message);

private:

    QWebSocket m_webSocket;
    QUrl m_url;

    bool isStopped = false;

    bool isConnected = false;

    std::vector<QString> messageQueue;

    QThread thread1;

    QString helloString;
};

#endif // WEBSOCKETCLIENT_H
