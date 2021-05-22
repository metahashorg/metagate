#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <memory>
#include <unordered_map>

#include "utilites/OopUtils.h"
#include "utilites/RequestId.h"

#include <QObject>
#include <QtNetwork/QLocalServer>
#include <QDataStream>
#include <QByteArray>

class QLocalSocket;

namespace localconnection
{

class LocalServerRequest: public no_copyable, public no_moveable {
public:

    LocalServerRequest(QLocalSocket *connection, const QByteArray &data);

    ~LocalServerRequest();

    QByteArray request() const;

    void response(const QByteArray &data);

private:

    bool sended = false;
    QByteArray requestData;
    QLocalSocket *connection = nullptr;
};

class LocalServer : public QObject {
    Q_OBJECT
public:
    explicit LocalServer(const QString &nameServer, QObject *parent = nullptr);

signals:

    void request(std::shared_ptr<LocalServerRequest> request);

private slots:

    void onNewConnection();

    void onTextMessageReceived(size_t id, QLocalSocket *socket);

private:

    struct Buffer {
        QDataStream dataStream;
        quint32 size = 0;
    };

private:

    std::unordered_map<size_t, Buffer> buffers;

    QLocalServer *server;

    RequestId id;
};

}

#endif // LOCALSERVER_H
