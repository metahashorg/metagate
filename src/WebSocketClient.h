#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QThread>
#include <map>
#include <QtWebSockets/QWebSocket>

#include "TimerClass.h"

class WebSocketClient : public TimerClass
{
    Q_OBJECT
public:
    explicit WebSocketClient(const QString &url, QObject *parent = nullptr);

    ~WebSocketClient();

    void start();

signals:

    void closed();

    void sendMessage(QString message);

    void sendMessages(const std::vector<QString> &messages);

    void setHelloString(QString message, QString tag);

    void setHelloString(const std::vector<QString> &messages, QString tag);

    void addHelloString(QString message, QString tag);

signals:

    void messageReceived(QString message);

public slots:

    void onStarted();

    void onConnected();
    void onTextMessageReceived(QString message);

    void onSendMessage(QString message);

    void onSendMessages(const std::vector<QString> &messages);

    void onSetHelloString(QString message, QString tag);

    void onSetHelloString(const std::vector<QString> &messages, QString tag);

    void onAddHelloString(QString message, QString tag);

private slots:

    void onTimerEvent();

    void onPong(quint64 elapsedTime, const QByteArray &payload);

private:

    void sendMessagesInternal();

private:

    QWebSocket m_webSocket;
    QUrl m_url;

    bool isStopped = false;

    bool isConnected = false;

    std::vector<QString> messageQueue;

    std::map<QString, std::vector<QString>> helloStrings;

    time_point prevPongTime;
};

#endif // WEBSOCKETCLIENT_H
