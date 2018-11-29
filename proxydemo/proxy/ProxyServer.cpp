#include "ProxyServer.h"

#include "ConnectionRunnable.h"
#include "ProxyClient.h"
#include <QThread>

namespace proxy
{

ProxyServer::ProxyServer(QObject *parent)
    : QTcpServer(parent)
{
    m_pool = new QThreadPool(this);
}

void ProxyServer::start()
{
    if (!this->listen(QHostAddress::Any, 1234))
        ;
}

void ProxyServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "connect";
    QThread *thread = new QThread();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    ProxyClient *client = new ProxyClient();
    //client->moveToThread(thread);
    thread->start();
    client->setSocketDescriptor(socketDescriptor);
    //ConnectionRunnable *task = new ConnectionRunnable(socketDescriptor);
    //task->setAutoDelete(true);
    //m_pool->start(task);
}

}
