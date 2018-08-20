#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>

#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

#include "duration.h"

using ClientCallback = std::function<void(const std::string &response)>;

using PingCallback = std::function<void(const QString &address, const milliseconds &time, const std::string &response)>;

using ReturnCallback = std::function<void()>;

/*
   На каждый поток должен быть один экземпляр класса.
   */
class SimpleClient : public QObject
{
    Q_OBJECT

private:

    using PingCallbackInternal = std::function<void(const milliseconds &time, const std::string &response)>;

public:

    static const std::string ERROR_BAD_REQUEST;

public:

    explicit SimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback);

    // ping хорошо работает только с максимум одним одновременным запросом
    void ping(const QString &address, const PingCallback &callback, milliseconds timeout);

    void setParent(QObject *obj);

    void moveToThread(QThread *thread);

Q_SIGNALS:

    void callbackCall(ReturnCallback callback);

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onTextMessageReceived();

    void onPingReceived();

    void onTimerEvent();

private:

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<class Callbacks, typename... Message>
    void runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages);

    void startTimer();

private:
    std::unique_ptr<QNetworkAccessManager> manager;
    std::unordered_map<std::string, ClientCallback> callbacks_;
    std::unordered_map<std::string, PingCallbackInternal> pingCallbacks_;

    std::unordered_map<std::string, QNetworkReply*> requests;

    QTimer* timer = nullptr;

    QThread *thread1 = nullptr;

    int id = 0;
};

#endif // CLIENT_H
