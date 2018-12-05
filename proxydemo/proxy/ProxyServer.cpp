#include "ProxyServer.h"

#include "ConnectionRunnable.h"
#include "ProxyClient.h"
#include <QThread>

namespace proxy
{

ProxyServer::ProxyServer(QObject *parent)
    : QTcpServer(parent)
{
}

void ProxyServer::start()
{
    if (!this->listen(QHostAddress::Any, 1234))
        ;
}

void ProxyServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "connect";
    QThread *thread = new QThread(this);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    ProxyClient *client = new ProxyClient();
    connect(client, &ProxyClient::destroyed, thread, &QThread::quit);
    client->setSocketDescriptor(socketDescriptor);
    client->moveToThread(thread);
    thread->start();
}

}
