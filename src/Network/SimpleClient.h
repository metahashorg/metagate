#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QUrl>

#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

#include "duration.h"

class QNetworkAccessManager;
class QTimer;
class QNetworkReply;

/*
   На каждый поток должен быть один экземпляр класса.
   */
class SimpleClient : public QObject {
    Q_OBJECT
public:

    struct ServerException {

        ServerException() = default;

        ServerException(const std::string &server, int code, const std::string &description, const std::string &content)
            : server(server)
            , description(description)
            , content(content)
            , code(code)
        {}

        std::string server;

        std::string description;

        std::string content;

        bool isSet() const {
            return code != 0;
        }

        bool isTimeout() const;

        std::string toString() const {
            return server + ". " + description + ". " + content + ".";
        }

        const static int BAD_REQUEST_ERROR;

        int code = 0;
    };

    struct Response {
        std::string response;
        ServerException exception;
        milliseconds time;
        bool isTimeout = false;
    };

public:

    using ClientCallback = std::function<void(const Response &response)>;

    using ClientCallbacks = std::function<void(const std::vector<Response> &response)>;

    using ReturnCallback = std::function<void()>;

public:

    explicit SimpleClient();

    ~SimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout, bool isClearCache=false);
    void sendMessagesPost(const std::string printedName, const std::vector<QUrl> &urls, const QString &message, const ClientCallbacks &callback, milliseconds timeout);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback, milliseconds timeout);

    void setParent(QObject *obj);

    void moveToThread(QThread *thread);

Q_SIGNALS:

    void callbackCall(SimpleClient::ReturnCallback callback);

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onTextMessageReceived();

    void onTimerEvent();

private:

    using TextMessageReceived = void (SimpleClient::*)();

    template<typename Callback>
    void sendMessageInternal(
        bool isPost,
        std::unordered_map<std::string, Callback> &callbacks,
        const QUrl &url,
        const QString &message,
        const Callback &callback,
        bool isTimeout,
        milliseconds timeout,
        bool isClearCache,
        TextMessageReceived onTextMessageReceived,
        bool isQueuedConnection
    );

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout, bool isClearCache);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<class Callbacks, typename... Message>
    void runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages);

    void startTimer1();

private:
    QNetworkAccessManager *manager;
    std::unordered_map<std::string, ClientCallback> callbacks_;

    std::unordered_map<std::string, QNetworkReply*> requests;

    QTimer* timer = nullptr;

    QThread *thread1 = nullptr;

    int id = 0;
};

#endif // CLIENT_H
