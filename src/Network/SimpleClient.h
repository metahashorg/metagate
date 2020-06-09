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

        const static int TIMEOUT_REQUEST_ERROR;

        int code = 0;
    };

    struct Response {
        QByteArray response;
        ServerException exception;
        milliseconds time;
    };

public:

    using ClientCallback = std::function<void(const Response &response)>;

    using ClientCallbacks = std::function<void(const std::vector<Response> &response)>;

    using ReturnCallback = std::function<void()>;

public:

    explicit SimpleClient(QObject *parent = nullptr);

    ~SimpleClient();

    void sendMessagePost(const QUrl &url, const QByteArray &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QByteArray &message, const ClientCallback &callback, milliseconds timeout, bool isClearCache=false);
    void sendMessagesPost(const std::string printedName, const std::vector<QUrl> &urls, const QByteArray &message, const ClientCallbacks &callback, milliseconds timeout);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback, milliseconds timeout);

    //void setParent(QObject *obj);

    //void moveToThread(QThread *thread);

Q_SIGNALS:

    void callbackCall(SimpleClient::ReturnCallback callback);

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onTextMessageReceived(size_t id);

    void onTimerEvent();

private:

    struct Request {
        ClientCallback callback;
        QNetworkReply* reply = nullptr;
        bool isSetTimeout = false;
        milliseconds timeout;
        time_point beginTime;
        bool isTimeout = false;
    };

private:

    template<typename Callback>
    void sendMessageInternal(
        bool isPost,
        const QUrl &url,
        const QByteArray &message,
        const Callback &callback,
        bool isTimeout,
        milliseconds timeout,
        bool isClearCache,
        bool isQueuedConnection
    );

    void sendMessagePost(const QUrl &url, const QByteArray &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout, bool isClearCache);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<typename... Message>
    void runCallback(size_t id, Message&&... messages);

    void startTimer1();

private:
    QNetworkAccessManager *manager;

    std::unordered_map<size_t, Request> requests;

    QTimer* timer = nullptr;

    QThread *thread1 = nullptr;

    size_t id = 0;
};

#endif // CLIENT_H
