#include "LocalClient.h"

#include <QLocalSocket>

using namespace std::placeholders;

#include "check.h"
#include "Log.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

LocalClient::LocalClient(const QString &localServerName)
    : QObject(nullptr)
    , localServerName(localServerName)
{
    Q_REG(LocalClient::ReturnCallback, "LocalClient::ReturnCallback");
}

void LocalClient::mvToThread(QThread *thread) {
    this->moveToThread(thread);
}

void LocalClient::sendMessage(const std::string &message, const LocalClient::ClientCallback &callback) {
    const size_t currId = id.get();
    callbacks[currId] = callback;

    QLocalSocket *socket = new QLocalSocket(this);

    Q_CONNECT(socket, &QLocalSocket::readyRead, this, std::bind(&LocalClient::onTextMessageReceived, this, currId));
    Q_CONNECT(socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), this, std::bind(&LocalClient::onErrorMessageReceived, this, currId, _1));

    QDataStream &outStream = buffers[currId].dataStream;
    outStream.setVersion(QDataStream::Qt_5_10);
    outStream.setDevice(socket);

    socket->connectToServer(localServerName);
    if (socket->isValid()) {
        QByteArray block;
        QDataStream inStream(&block, QIODevice::WriteOnly);
        inStream << (qint32)(message.size());
        inStream.writeRawData(message.data(), message.size());
        socket->write(block);
        socket->flush();
    }
}

template<typename... Message>
void LocalClient::runCallback(size_t id, Message&&... messages) {
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + std::to_string(id));
    const auto callback = std::bind(foundCallback->second, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
    buffers.erase(id);
}

void LocalClient::onTextMessageReceived(size_t id) {
BEGIN_SLOT_WRAPPER
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());

    Buffer &currentBuffer = buffers[id];

    if (currentBuffer.size == 0) {
        if (socket->bytesAvailable() < (int)sizeof(quint32)) {
            return;
        }

        currentBuffer.dataStream >> currentBuffer.size;
    }

    if (socket->bytesAvailable() < currentBuffer.size || currentBuffer.dataStream.atEnd()) {
        return;
    }

    QByteArray data;
    currentBuffer.dataStream >> data;

    Response resp;
    resp.response = std::string(data.data(), data.size());

    runCallback(id, resp);

    socket->deleteLater();
END_SLOT_WRAPPER
}

void LocalClient::onErrorMessageReceived(size_t id, QLocalSocket::LocalSocketError socketError) {
BEGIN_SLOT_WRAPPER
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());

    Response resp;

    switch (socketError) {
    case QLocalSocket::ServerNotFoundError: {
        resp.exception = ServerException(1, "Server not found");
        break;
    } case QLocalSocket::ConnectionRefusedError: {
        resp.exception = ServerException(2, "Connection refused");
        break;
    } case QLocalSocket::PeerClosedError: {
        resp.exception = ServerException(3, "Peer closed");
        break;
    } default: {
        resp.exception = ServerException(4, socket->errorString().toStdString());
    }
    }

    runCallback(id, resp);

    socket->deleteLater();
END_SLOT_WRAPPER
}
