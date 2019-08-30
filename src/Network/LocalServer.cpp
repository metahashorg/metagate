#include "LocalServer.h"

#include <QLocalSocket>

#include "utilites/utils.h"

#include "check.h"
#include "Log.h"
#include "qt_utilites/QRegister.h"

std::string LocalServerRequest::request() const {
    CHECK(connection != nullptr, "Connection nullptr");
    const QByteArray data = connection->readAll();
    return std::string(data.data(), data.size());
}

void LocalServerRequest::response(const std::string &data) {
    CHECK(connection != nullptr, "Connection nullptr");

    CHECK(!sended, "Already sended");

    connection->write(data.data(), data.size());
    connection->flush();
    connection->disconnectFromServer();

    sended = true;
}

LocalServerRequest::LocalServerRequest(QLocalSocket *connection)
    : connection(connection)
{}

LocalServerRequest::~LocalServerRequest() {
    if (connection == nullptr) {
        return;
    }
    if (!sended) {
        LOG << "Warn. Not sended response";
        response("Internal server error: Empty response");
    }
}

LocalServer::LocalServer(const QString &nameServer)
    : QObject(nullptr)
{
    const QString listen = "/tmp/" + nameServer;
#ifndef TARGET_WINDOWS
    removeFile(listen);
#endif // TARGET_WINDOWS
    CHECK(server.listen(listen), "Error listen local server: " + server.errorString().toStdString());

    Q_CONNECT(&server, &QLocalServer::newConnection, this, &LocalServer::newConnection);

    Q_REG(std::shared_ptr<LocalServerRequest>, "std::shared_ptr<LocalServerRequest>");
}

void LocalServer::mvToThread(QThread *th) {
    moveToThread(th);
    server.moveToThread(th);
}

void LocalServer::newConnection() {
    QLocalSocket *clientConnection = server.nextPendingConnection();
    if (clientConnection == nullptr) {
        return;
    }
    Q_CONNECT(clientConnection, &QLocalSocket::disconnected, clientConnection, &QLocalSocket::deleteLater);

    Q_CONNECT3(clientConnection, &QLocalSocket::readyRead, ([this, clientConnection](){
        emit request(std::make_shared<LocalServerRequest>(clientConnection));
    }));
}
