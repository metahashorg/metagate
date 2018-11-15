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

/*
   На каждый поток должен быть один экземпляр класса.
   */
class SimpleClient : public QObject
{
    Q_OBJECT

public:

    struct ServerException {

        ServerException() = default;

        ServerException(int code, const std::string &description, const std::string &content)
            : code(code)
            , description(description)
            , content(content)
        {}

        std::string description;

        std::string content;

        bool isSet() const {
            return code != 0;
        }

        const static int BAD_REQUEST_ERROR;

        int code = 0;
    };

public:

    using ClientCallback = std::function<void(const std::string &response, const ServerException &exception)>;

    using PingCallback = std::function<void(const QString &address, const milliseconds &time, const std::string &response)>;

    using ReturnCallback = std::function<void()>;

private:

    using PingCallbackInternal = std::function<void(const milliseconds &time, const std::string &response)>;

public:

    explicit SimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback, milliseconds timeout);

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
    void sendMessageGet(const QUrl &url, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<class Callbacks, typename... Message>
    void runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages);

    void startTimer1();

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
