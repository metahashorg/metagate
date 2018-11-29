#include "ProxyClient.h"

#include <QNetworkReply>
#include <QEvent>
#include <QThread>
#include <QDebug>

ProxyClient::ProxyClient(QObject *parent)
    : QTcpSocket(parent)
{
    qDebug() << "create " << QThread::currentThread();

    connect(this, &QAbstractSocket::connected, this, &ProxyClient::onConnected);
    connect(this, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &ProxyClient::onError);
    connect(this, &QIODevice::readyRead, this, &ProxyClient::onReadyRead);

    startNewQuery();
}

bool ProxyClient::event(QEvent *e)
{
    qDebug() << e->type();
    return QTcpSocket::event(e);
}

void ProxyClient::onConnected()
{
    qDebug() << "connected";
}

void ProxyClient::onError(QAbstractSocket::SocketError socketError)
{

}

void ProxyClient::onReadyRead()
{
    QByteArray d = readAll();
    qDebug() << d;
    m_data += d;
    parse();
    /*if (m_error) {
        abort();
        emit finished();
    }
    if (m_headerParsed) {
        if (m_contentLength != -1) {
            if (m_data.length() >= m_contentLength) {
                m_reply = m_data.left(m_contentLength);
                emit finished();
            }
        } else {
            m_error = true;
            abort();
            emit finished();
        }

    }
    */
}

void ProxyClient::startNewQuery()
{
    m_headerParsed = false;
    m_contentLength = -1;
    m_firstHeaderString = QByteArray();
    m_headers.clear();
    m_body = QByteArray();
}

void ProxyClient::parse()
{
    parseHeader();
    if (m_headerParsed) {
        if (m_body.length() < m_contentLength) {
            m_body += m_data;
            m_data.clear();
        }
        if (m_body.length() >= m_contentLength) {
            sendQuery();
            startNewQuery();
        }

    }
}

void ProxyClient::parseHeader()
{
    if (m_headerParsed)
        return;
    while(true) {
        int index = m_data.indexOf('\n');
        if (index == -1)
            break;
        QByteArray s = m_data.left(index - 1);
        m_data = m_data.mid(index + 1);
        qDebug() << s;
        if (s.isEmpty() || s == QByteArray("\r")) {
            m_headerParsed = true;
            return;
        }
        if (m_firstHeaderString.isEmpty()) {
            /*if (!s.startsWith("HTTP/1.1 200 OK") && !s.startsWith("HTTP/1.0 200 OK")) {
                // HTTP error
                //m_error = true;
                return;
            }*/
            m_firstHeaderString = s;
        } else {
            m_headers.append(s);
        }

        if (s.startsWith("Content-Length: ")) {
            s = s.mid(16);
            if (s.endsWith('\r'))
                s = s.left(s.length() - 1);
            m_contentLength = s.toInt();
        }
    }
}

void ProxyClient::sendQuery()
{
    qDebug() << "Q " << m_firstHeaderString;
    int index1 = m_firstHeaderString.indexOf(' ');
    if (index1 == -1)
        return;
    QByteArray method = m_firstHeaderString.left(index1);
    int index2 = m_firstHeaderString.indexOf(' ', index1 + 1);
    if (index2 == -1)
        return;
    QByteArray url = m_firstHeaderString.mid(index1 + 1, index2 - index1 - 1);
    qDebug() << url;
    QNetworkRequest req(QString::fromLatin1(url));
    for (const QByteArray &h : m_headers) {
        int idx = h.indexOf(":");
        if (idx == -1)
            continue;
        QByteArray name = h.left(idx);
        QByteArray value = h.right(h.length() - idx - 1);
        qDebug() << name << " -> " << value;
        if (name == "Proxy-Connection")
            continue;
        if (name == "Content-Length")
                continue;
        req.setRawHeader(name, value);
    }
    QNetworkReply *reply = nullptr;
    if (method == "GET")
        reply= m_manager.get(req);
    else if (method == "POST")
        reply= m_manager.post(req, m_body);
    QObject::connect(reply, &QNetworkReply::finished,
                     [reply, this]() {
        if (reply->error()) {
            //m_error = "Failed to download %1: %2", d->location.toDisplayString(), j->errorString());
            //LOG
            qDebug() << reply->errorString();
            return;
        }

        this->write("HTTP/1.1 200 OK\r\n");

        this->write(QByteArray("Content-Length: ") + QByteArray::number(reply->bytesAvailable()) + QByteArray("\r\n\r\n"));
        this->write(reply->readAll());
        //this->write("\r\n");
        this->flush();
        reply->deleteLater();
    });
}
