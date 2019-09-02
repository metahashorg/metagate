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

    socket->connectToServer(localServerName);
    if (socket->isValid()) {
        socket->write(message.data(), message.size());
    }
}

template<typename... Message>
void LocalClient::runCallback(size_t id, Message&&... messages) {
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + std::to_string(id));
    const auto callback = std::bind(foundCallback->second, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
}

void LocalClient::onTextMessageReceived(size_t id) {
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket->bytesAvailable() < (int)sizeof(quint32))
        return;
    QByteArray ss = socket->readAll();

    Response resp;
    resp.response = std::string(ss.data(), ss.size());

    runCallback(id, resp);

    socket->deleteLater();
}

void LocalClient::onErrorMessageReceived(size_t id, QLocalSocket::LocalSocketError socketError) {
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
}
