#include "ProxyClient.h"

#include <QNetworkReply>
#include <QEvent>
#include <QThread>
#include <QDebug>

#include "http_parser.h"

#include <iostream>

class ProxyClientPrivate
{
public:
    enum Method {Get, Post, Connect};

    ProxyClientPrivate(QTcpSocket *parent);

    void parseRequestData(const QByteArray &data);
    void parseRespData(const QByteArray &data);

    void startQuery(const QByteArray &method, const QUrl &url);
    void sendHeader(const QByteArray &name, const QByteArray &value);
    void headerComplete();
    void sendBody(const QByteArray &data);
    void connectionEstablished();

    static int reqOnMessageBegin(http_parser* p);
    static int reqOnHeadersComplete(http_parser* p);
    static int reqOnMessageComplete(http_parser* p);
    static int reqOnChunkHeader(http_parser* p);
    static int reqOnChunkComplete(http_parser* p);
    static int reqOnHeaderField(http_parser *p, const char *at, size_t length);
    static int reqOnHeaderValue(http_parser *p, const char *at, size_t length);
    static int reqOnUrl(http_parser *p, const char *at, size_t length);
    static int reqOnStatus(http_parser* p, const char *at, size_t length);
    static int reqOnBody(http_parser* p, const char *at, size_t length);

    static int respOnMessageBegin(http_parser* p);
    static int respOnHeadersComplete(http_parser* p);
    static int respOnMessageComplete(http_parser* p);
    static int respOnChunkHeader(http_parser* p);
    static int respOnChunkComplete(http_parser* p);
    static int respOnHeaderField(http_parser *p, const char *at, size_t length);
    static int respOnHeaderValue(http_parser *p, const char *at, size_t length);
    static int respOnUrl(http_parser *p, const char *at, size_t length);
    static int respOnStatus(http_parser* p, const char *at, size_t length);
    static int respOnBody(http_parser* p, const char *at, size_t length);

    QByteArray lastHeaderField;
    http_parser reqParser;
    http_parser respParser;
    http_parser_settings reqSettings;
    http_parser_settings respSettings;
    QTcpSocket *srcSocket = nullptr;
    QTcpSocket *socket = nullptr;
};

ProxyClientPrivate::ProxyClientPrivate(QTcpSocket *parent)
    : srcSocket(parent)
    , socket(new QTcpSocket(parent))
{
    http_parser_init(&reqParser, HTTP_REQUEST);
    reqParser.data = this;

    reqSettings.on_message_begin = reqOnMessageBegin;
    reqSettings.on_headers_complete = reqOnHeadersComplete;
    reqSettings.on_message_complete = reqOnMessageComplete;
    reqSettings.on_header_field = reqOnHeaderField;
    reqSettings.on_header_value = reqOnHeaderValue;
    reqSettings.on_url = reqOnUrl;
    reqSettings.on_status = reqOnStatus;
    reqSettings.on_body = reqOnBody;
    reqSettings.on_chunk_header = reqOnChunkHeader;
    reqSettings.on_chunk_complete = reqOnChunkComplete;

    http_parser_init(&respParser, HTTP_RESPONSE);
    respParser.data = this;
    respSettings.on_message_begin = respOnMessageBegin;
    respSettings.on_headers_complete = respOnHeadersComplete;
    respSettings.on_message_complete = respOnMessageComplete;
    respSettings.on_header_field = respOnHeaderField;
    respSettings.on_header_value = respOnHeaderValue;
    respSettings.on_url = respOnUrl;
    respSettings.on_status = respOnStatus;
    respSettings.on_body = respOnBody;
    respSettings.on_chunk_header = respOnChunkHeader;
    respSettings.on_chunk_complete = respOnChunkComplete;

}

void ProxyClientPrivate::parseRequestData(const QByteArray &data)
{
    size_t parsed;
    parsed = http_parser_execute(&reqParser, &reqSettings, data.constData(), data.size());
    qDebug() << "parsed" << parsed;
}

void ProxyClientPrivate::parseRespData(const QByteArray &data)
{
    size_t parsed;
    parsed = http_parser_execute(&respParser, &respSettings, data.constData(), data.size());
    qDebug() << "parsed" << parsed;
}

void ProxyClientPrivate::startQuery(const QByteArray &method, const QUrl &url)
{
    //quint16 port = url.port();
    qDebug() << "Method" << url;
    QUrl u(url);
    u.setScheme("http");
    qDebug() << u.host() << u.port(80);
    socket->connectToHost(u.host(), u.port(80));
    if (socket->waitForConnected()) {
        qDebug() << "connected";
    } else {
        qDebug() << "error";
    }
    if (method == QByteArrayLiteral("CONNECT")) {
        connectionEstablished();
        return;
    }
    QString header = QStringLiteral("%1 %2 HTTP/1.1\r\n").arg(QString::fromLatin1(method)).arg(url.path());
    socket->write(header.toLatin1());
}

void ProxyClientPrivate::sendHeader(const QByteArray &name, const QByteArray &value)
{
    if (name == QByteArrayLiteral("Proxy-Connection"))
        return;
    socket->write(name + QByteArray(": ") + value + QByteArray("\r\n"));
}

void ProxyClientPrivate::headerComplete()
{
    socket->write(QByteArray("\r\n"));

}

void ProxyClientPrivate::sendBody(const QByteArray &data)
{
    socket->write(data);
}

void ProxyClientPrivate::connectionEstablished()
{
    qDebug() << "send";
    srcSocket->write(QByteArray("HTTP/1.0 200 Connection established\r\n"));
    srcSocket->write(QByteArray("MetaGate Proxy\r\n"));
    srcSocket->flush();
    qDebug() << srcSocket->waitForBytesWritten();
    qDebug() << srcSocket->errorString();
}

int ProxyClientPrivate::reqOnMessageBegin(http_parser *p)
{
    qDebug() << "on_message_begin";
    return 0;
}

int ProxyClientPrivate::reqOnHeadersComplete(http_parser *p)
{
    qDebug() << "on_headers_complete";
    static_cast<ProxyClientPrivate *>(p->data)->headerComplete();
    return 0;
}

int ProxyClientPrivate::reqOnMessageComplete(http_parser *p)
{
    qDebug() << "on_message_complete";
    qDebug() << (void *)p->sbody;
    qDebug() << (void *)p->ptr;
    qDebug() << "body size = " << (quint64)(p->ptr - p->sbody);
    QByteArray body(p->sbody + 1, (int)(p->ptr - p->sbody));
    qDebug() << body;
    static_cast<ProxyClientPrivate *>(p->data)->sendBody(body);
    return 0;
}

int ProxyClientPrivate::reqOnChunkHeader(http_parser *p)
{
    qDebug() << "on_chunk_header";
    return 0;
}

int ProxyClientPrivate::reqOnChunkComplete(http_parser *p)
{
    qDebug() << "on_chunk_complete";
    return 0;
}

int ProxyClientPrivate::reqOnHeaderField(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    qDebug() << "on_header_field " << data;
    static_cast<ProxyClientPrivate *>(p->data)->lastHeaderField = data;
    return 0;
}

int ProxyClientPrivate::reqOnHeaderValue(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    qDebug() << "on_header_value " << data;
    static_cast<ProxyClientPrivate *>(p->data)->sendHeader(static_cast<ProxyClientPrivate *>(p->data)->lastHeaderField, data);
    return 0;
}

int ProxyClientPrivate::reqOnUrl(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    qDebug() << "on_url " << data;
    QUrl url(QString::fromLatin1(data));
    QByteArray method(http_method_str((enum http_method)p->method));
    static_cast<ProxyClientPrivate *>(p->data)->startQuery(method, url);
    return 0;
}

int ProxyClientPrivate::reqOnStatus(http_parser *p, const char *at, size_t length)
{
    QByteArray a(at, length);
    qDebug() << "on_status " << a;
    return 0;
}

int ProxyClientPrivate::reqOnBody(http_parser *p, const char *at, size_t length)
{
    QByteArray a(at, length);
    qDebug() << "on_body " << a;
    return 0;
}

/////////////////////

int ProxyClientPrivate::respOnMessageBegin(http_parser* p)
{
    qDebug() << "on_message_begin";
    return 0;
}

int ProxyClientPrivate::respOnHeadersComplete(http_parser* p)
{
    qDebug() << "on_headers_complete";
    //static_cast<ProxyClient *>(p->data)->headerComplete();
    return 0;
}

int ProxyClientPrivate::respOnMessageComplete(http_parser* p)
{
    qDebug() << "on_message_complete";
    return 0;
}

int ProxyClientPrivate::respOnChunkHeader(http_parser* p)
{
    qDebug() << "on_chunk_header";
    return 0;
}

int ProxyClientPrivate::respOnChunkComplete(http_parser* p)
{
    qDebug() << "on_chunk_complete";
    return 0;
}

int ProxyClientPrivate::respOnHeaderField(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    qDebug() << "on_header_field " << data;
    //lastHeaderField = data;
    return 0;
}

int ProxyClientPrivate::respOnHeaderValue(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    qDebug() << "on_header_value " << data;
    //static_cast<ProxyClient *>(p->data)->sendHeader(lastHeaderField, data);
    return 0;
}

int ProxyClientPrivate::respOnUrl(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    qDebug() << "on_url " << data;
    //QUrl url(QString::fromLatin1(data));
    //QByteArray method(http_method_str((enum http_method)p->method));
    //static_cast<ProxyClient *>(p->data)->startQuery(method, url);
    return 0;
}

int ProxyClientPrivate::respOnStatus(http_parser* p, const char *at, size_t length)
{
    QByteArray a(at, length);
    qDebug() << "on_status " << a;
    return 0;
}

int ProxyClientPrivate::respOnBody(http_parser* p, const char *at, size_t length)
{
    QByteArray a(at, length);
    qDebug() << "on_body " << a;
    return 0;
}






void ProxyClient::parseResp(const QByteArray &data)
{
    d->parseRespData(data);
}


ProxyClient::ProxyClient(QObject *parent)
    : QTcpSocket(parent)
    , d(new ProxyClientPrivate(this))
{
    qDebug() << "create " << QThread::currentThread();

    connect(this, &QAbstractSocket::connected, this, &ProxyClient::onConnected);
    connect(this, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &ProxyClient::onError);
    connect(this, &QIODevice::readyRead, this, &ProxyClient::onReadyRead);

    connect(d->socket, &QIODevice::readyRead, [this]() {
        QByteArray data = d->socket->readAll();
        qDebug() << "RRRR\n" << data;
        parseResp(data);
        d->srcSocket->write(data);
        d->srcSocket->flush();
        d->srcSocket->waitForBytesWritten();
    });
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
    QByteArray data = readAll();
    qDebug() << data;
    //write(QByteArray("Hello"));
    d->parseRequestData(data);

}
