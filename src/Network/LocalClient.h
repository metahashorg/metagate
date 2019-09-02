#ifndef LOCALCLIENT_H
#define LOCALCLIENT_H

#include <QObject>
#include <QLocalSocket>

#include <functional>
#include <unordered_map>

#include "utilites/RequestId.h"

class LocalClient: public QObject {
    Q_OBJECT
public:

    struct ServerException {

        ServerException() = default;

        ServerException(int code, const std::string &description)
            : description(description)
            , code(code)
        {}

        std::string description;

        std::string content;

        bool isSet() const {
            return code != 0;
        }

        std::string toString() const {
            return description;
        }

        int code = 0;
    };

    struct Response {
        std::string response;
        ServerException exception;
    };

public:

    using ClientCallback = std::function<void(const Response &response)>;

    using ReturnCallback = std::function<void()>;

public:

    explicit LocalClient(const QString &localServerName);

    void mvToThread(QThread *thread);

public:

    void sendMessage(const std::string &message, const LocalClient::ClientCallback &callback);

signals:

    void callbackCall(LocalClient::ReturnCallback callback);

private slots:

    void onTextMessageReceived(size_t id);

    void onErrorMessageReceived(size_t id, QLocalSocket::LocalSocketError socketError);

private:

    template<typename... Message>
    void runCallback(size_t id, Message&&... messages);

private:

    QString localServerName;

    std::unordered_map<size_t, ClientCallback> callbacks;

    RequestId id;
};

#endif // LOCALCLIENT_H
