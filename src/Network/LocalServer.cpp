#include "LocalServer.h"

#include <QLocalSocket>

#include "utilites/utils.h"

#include "Log.h"
#include "check.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/SlotWrapper.h"

namespace localconnection
{

QByteArray LocalServerRequest::request() const
{
    return requestData;
}

void LocalServerRequest::response(const QByteArray& data)
{
    CHECK(connection != nullptr, "Connection nullptr");

    CHECK(!sended, "Already sended");
    qDebug() << data;

    QByteArray block;
    QDataStream inStream(&block, QIODevice::WriteOnly);
    inStream.setVersion(QDataStream::Qt_5_10);
    inStream << static_cast<qint32>(data.size());
    inStream << data;
    qDebug() << data.size();
    qDebug() << block.size();
    // const int res = inStream.writeBytes( .writeRawData(data.data(),
    // data.size()); CHECK(res != -1, "Dont write response to localserver");

    connection->write(block);
    connection->flush();
    connection->disconnectFromServer();

    sended = true;
}

LocalServerRequest::LocalServerRequest(
    QLocalSocket* connection, const QByteArray& data) :
  requestData(data), connection(connection)
{
}

LocalServerRequest::~LocalServerRequest()
{
    if (connection == nullptr)
    {
        return;
    }
    if (!sended)
    {
        LOG << "Warn. Not sended response";
        response("Internal server error: Empty response");
    }
}

LocalServer::LocalServer(const QString& nameServer, QObject* parent) :
  QObject(parent), server(new QLocalServer(this))
{
    QLocalServer::removeServer(nameServer);
    server->setSocketOptions(QLocalServer::WorldAccessOption);
    CHECK(
        server->listen(nameServer),
        "Error listen local server: " + server->errorString().toStdString());

    Q_CONNECT(
        server,
        &QLocalServer::newConnection,
        this,
        &LocalServer::onNewConnection);

    Q_REG(
        std::shared_ptr<LocalServerRequest>,
        "std::shared_ptr<LocalServerRequest>");
}

void LocalServer::onNewConnection()
{
    BEGIN_SLOT_WRAPPER
    QLocalSocket* clientConnection = server->nextPendingConnection();
    if (clientConnection == nullptr)
    {
        return;
    }

    const size_t currId = id.get();
    QDataStream& outStream = buffers[currId].dataStream;
    outStream.setVersion(QDataStream::Qt_5_10);
    outStream.setDevice(clientConnection);

    Q_CONNECT(
        clientConnection,
        &QLocalSocket::disconnected,
        clientConnection,
        &QLocalSocket::deleteLater);

    Q_CONNECT3(
        clientConnection,
        &QLocalSocket::readyRead,
        std::bind(
            &LocalServer::onTextMessageReceived,
            this,
            currId,
            clientConnection));
    END_SLOT_WRAPPER
}

void LocalServer::onTextMessageReceived(size_t id, QLocalSocket* socket)
{
    BEGIN_SLOT_WRAPPER
    CHECK(buffers.find(id) != buffers.end(), "Incorrect request from client");
    Buffer& currentBuffer = buffers[id];

    if (currentBuffer.size == 0)
    {
        if (socket->bytesAvailable() < (int)sizeof(quint32))
        {
            return;
        }

        currentBuffer.dataStream >> currentBuffer.size;
    }

    if (socket->bytesAvailable() < currentBuffer.size
        || currentBuffer.dataStream.atEnd())
    {
        return;
    }

    QByteArray data;
    currentBuffer.dataStream >> data;

    emit request(std::make_shared<LocalServerRequest>(socket, data));

    buffers.erase(id);
    END_SLOT_WRAPPER
}

} // namespace localconnection
