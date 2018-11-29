#include "ConnectionRunnable.h"

#include <QThread>
#include <QTcpSocket>

#include <QEventLoop>
#include "ProxyClient.h"

namespace proxy
{

ConnectionRunnable::ConnectionRunnable(qintptr socketDescriptor)
    : m_socketDescriptor(socketDescriptor)
{

}

void ConnectionRunnable::run()
{
    if(!m_socketDescriptor)
        return;

    QEventLoop loop;
    qDebug() << "thread " << QThread::currentThread();

    ProxyClient *client = new ProxyClient();
    client->moveToThread(QThread::currentThread());
    client->setSocketDescriptor(m_socketDescriptor);

    client->write("HTTP/1.1 200 OK\r\n\r\nHello!\r\n");

    client->flush();

    loop.exec();
    /*QTcpSocket socket;
    socket.setSocketDescriptor(m_socketDescriptor);

    socket.waitForBytesWritten();
    qDebug() << socket.readAll();
    socket.write("HTTP/1.1 200 OK\r\n\r\nHello!\r\n");

    socket.flush();
    socket.waitForBytesWritten();
    socket.close();*/

}

}
