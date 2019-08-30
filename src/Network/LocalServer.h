#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <string>
#include <memory>

#include "utilites/OopUtils.h"

#include <QObject>
#include <QtNetwork/QLocalServer>

class QLocalSocket;

class LocalServerRequest: public no_copyable, public no_moveable {
public:

    LocalServerRequest(QLocalSocket *connection);

    ~LocalServerRequest();

    std::string request() const;

    void response(const std::string &data);

private:

    bool sended = false;
    QLocalSocket *connection = nullptr;
};

class LocalServer : public QObject {
    Q_OBJECT
public:
    explicit LocalServer(const QString &nameServer);

    void mvToThread(QThread *th);

signals:

    void request(std::shared_ptr<LocalServerRequest> request);

private slots:

    void newConnection();

private:

    QLocalServer server;
};

#endif // LOCALSERVER_H
