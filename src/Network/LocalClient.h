#ifndef LOCALCLIENT_H
#define LOCALCLIENT_H

#include <QObject>
#include <QLocalSocket>
#include <QDataStream>

#include <functional>
#include <unordered_map>

#include "utilites/RequestId.h"

namespace localconnection
{

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

        bool isSet() const {
            return code != 0;
        }

        std::string toString() const {
            return description;
        }

        int code = 0;
    };

    struct Response {
        QByteArray response;
        ServerException exception;
    };

public:

    using ClientCallback = std::function<void(const Response &response)>;

    using ReturnCallback = std::function<void()>;

public:

    explicit LocalClient(const QString &localServerName, QObject *parent = nullptr);

public:

    void sendRequest(const QByteArray &request, const LocalClient::ClientCallback &callback);

signals:

    void callbackCall(LocalClient::ReturnCallback callback);

private slots:

    void onTextMessageReceived(size_t id);

    void onErrorMessageReceived(size_t id, QLocalSocket::LocalSocketError socketError);

private:

    template<typename... Message>
    void runCallback(size_t id, Message&&... messages);

private:

    struct Buffer {
        QDataStream dataStream;
        quint32 size = 0;
    };

private:

    QString localServerName;

    std::unordered_map<size_t, ClientCallback> callbacks;

    std::unordered_map<size_t, Buffer> buffers;

    RequestId id;
};

}

#endif // LOCALCLIENT_H
