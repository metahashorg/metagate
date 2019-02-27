#include "ProxyClient.h"

#include <QNetworkReply>
#include <QThread>
#include <QHostAddress>
#include <QDebug>

#include "http_parser.h"
#include "check.h"
#include "SlotWrapper.h"

SET_LOG_NAMESPACE("PRX");

namespace proxy {

const QString error500("HTTP/1.0 500 Unable to connect\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: %1\r\n"
                          "Connection: close\r\n"
                          "\r\n");

const QByteArray error500Html("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                                "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
                                "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
                                "<html>\n"
                                "<head><title>500 Unable to connect</title></head>\n"
                                "<body>\n"
                                "<h1>Unable to connect</h1>\n"
                                "<p>MetaGate was unable to connect to the remote web server.</p>\n"
                                "</body>\n"
                                "</html>\n");

class ProxyClientPrivate
{
public:
    //enum Method {Get, Post, Connect};
    enum Result {No, GetPostQuery, ConnectQuery, NotConnected, ParseError};

    ProxyClientPrivate(ProxyClient *parent);

    void parseRequestData(const QByteArray &data);
//    void parseRespData(const QByteArray &data);

    void newQuery();
    void startQuery(const QByteArray &method, const QUrl &url);
    void sendHeader(const QByteArray &name, const QByteArray &value);
    void headerComplete();
    void sendBody(const QByteArray &data);
    void connectionEstablished();
    void sendErrorPage();

    static int reqOnMessageBegin(http_parser* p);
    static int reqOnHeadersComplete(http_parser* p);
    static int reqOnMessageComplete(http_parser* p);
    static int reqOnHeaderField(http_parser *p, const char *at, size_t length);
    static int reqOnHeaderValue(http_parser *p, const char *at, size_t length);
    static int reqOnUrl(http_parser *p, const char *at, size_t length);
    static int reqOnBody(http_parser *, const char *, size_t);


//    static int respOnMessageBegin(http_parser* p);
//    static int respOnHeadersComplete(http_parser* p);
//    static int respOnMessageComplete(http_parser* p);
//    static int respOnChunkHeader(http_parser* p);
//    static int respOnChunkComplete(http_parser* p);
//    static int respOnHeaderField(http_parser *p, const char *at, size_t length);
//    static int respOnHeaderValue(http_parser *p, const char *at, size_t length);
//    static int respOnUrl(http_parser *p, const char *at, size_t length);
//    static int respOnStatus(http_parser* p, const char *at, size_t length);
//    static int respOnBody(http_parser* p, const char *at, size_t length);

    QByteArray lastHeaderField;
    http_parser reqParser;
//    http_parser respParser;
    http_parser_settings reqSettings;
//    http_parser_settings respSettings;
    ProxyClient *srcSocket = nullptr;
    QTcpSocket *socket = nullptr;
    Result result;
    QString host;
    int port;
};


ProxyClientPrivate::ProxyClientPrivate(ProxyClient *parent)
    : srcSocket(parent)
    , result(No)
    //, socket(new QTcpSocket(parent->parent()))
{
    http_parser_init(&reqParser, HTTP_REQUEST);
    reqParser.data = this;

    reqSettings.on_message_begin = reqOnMessageBegin;
    reqSettings.on_headers_complete = reqOnHeadersComplete;
    reqSettings.on_message_complete = reqOnMessageComplete;
    reqSettings.on_header_field = reqOnHeaderField;
    reqSettings.on_header_value = reqOnHeaderValue;
    reqSettings.on_url = reqOnUrl;
    reqSettings.on_body = reqOnBody;

//    http_parser_init(&respParser, HTTP_RESPONSE);
//    respParser.data = this;
//    respSettings.on_message_begin = respOnMessageBegin;
//    respSettings.on_headers_complete = respOnHeadersComplete;
//    respSettings.on_message_complete = respOnMessageComplete;
//    respSettings.on_header_field = respOnHeaderField;
//    respSettings.on_header_value = respOnHeaderValue;
//    respSettings.on_url = respOnUrl;
//    respSettings.on_status = respOnStatus;
//    respSettings.on_body = respOnBody;
//    respSettings.on_chunk_header = respOnChunkHeader;
//    respSettings.on_chunk_complete = respOnChunkComplete;
}

void ProxyClientPrivate::parseRequestData(const QByteArray &data)
{
    size_t parsed;
    parsed = http_parser_execute(&reqParser, &reqSettings, data.constData(), data.size());
    if(parsed != data.size())
        result = ParseError;
}

//void ProxyClientPrivate::parseRespData(const QByteArray &data)
//{
//    size_t parsed;
//    parsed = http_parser_execute(&respParser, &respSettings, data.constData(), data.size());
//    qDebug() << "parsed" << parsed;
//}

void ProxyClientPrivate::newQuery()
{
    qDebug() << "Proxy, new query, connection from " << srcSocket->peerName();
}

void ProxyClientPrivate::startQuery(const QByteArray &method, const QUrl &url)
{
    QUrl u(url);
    u.setScheme("http");
    qDebug() << u.host() << u.port(80);
    host = u.host();
    port = u.port(80);

    if (method == QByteArrayLiteral("CONNECT")) {
        result = ConnectQuery;
        return;
    }

    if (!socket->isOpen()) {
        socket->connectToHost(host, port);
        if (!socket->waitForConnected()) {
            qDebug() << "not connected";
            result = NotConnected;
            return;
        }
    }
    QString header = QStringLiteral("%1 %2 HTTP/1.1\r\n")
            .arg(QString::fromLatin1(method))
            .arg(url.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority));
    socket->write(header.toLatin1());
}

void ProxyClientPrivate::sendHeader(const QByteArray &name, const QByteArray &value)
{
    if (!socket->isOpen())
        return;
    if (name == QByteArrayLiteral("Proxy-Connection"))
        return;
    if (name == QByteArrayLiteral("X-Forwarded-For"))
        return;
    if (name == QByteArrayLiteral("X-Real-IP"))
        return;
    socket->write(name + QByteArrayLiteral(": ") + value + QByteArrayLiteral("\r\n"));
}

void ProxyClientPrivate::headerComplete()
{
    if (!socket->isOpen())
        return;
    socket->write(QByteArrayLiteral("\r\n"));
}

void ProxyClientPrivate::sendBody(const QByteArray &data)
{
    if (!socket->isOpen())
        return;
    socket->write(data);
    socket->flush();
}

void ProxyClientPrivate::connectionEstablished()
{
    qDebug() << "send";
    srcSocket->write(QByteArray("HTTP/1.0 200 Connection established\r\n"));
    srcSocket->write(QByteArray("Proxy-agent: MetaGate Proxy\r\n"));
    srcSocket->write(QByteArray("\r\n"));
    srcSocket->flush();
}

void ProxyClientPrivate::sendErrorPage()
{
    srcSocket->write(error500.arg(error500Html.size()).toLatin1());
    srcSocket->write(error500Html);
    srcSocket->flush();
}

int ProxyClientPrivate::reqOnMessageBegin(http_parser *p)
{
    //qDebug() << "on_message_begin";
    static_cast<ProxyClientPrivate *>(p->data)->newQuery();
    return 0;
}

int ProxyClientPrivate::reqOnHeadersComplete(http_parser *p)
{
    //qDebug() << "on_headers_complete";
    static_cast<ProxyClientPrivate *>(p->data)->headerComplete();
    return 0;
}

int ProxyClientPrivate::reqOnMessageComplete(http_parser *p)
{
    //qDebug() << "on_message_complete";
    //qDebug() << "body size = " << (quint64)(p->ptr - p->sbody);
    QByteArray body(p->sbody + 1, (int)(p->ptr - p->sbody));
    //qDebug() << body;
    static_cast<ProxyClientPrivate *>(p->data)->sendBody(body);
    return 0;
}

int ProxyClientPrivate::reqOnHeaderField(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    //qDebug() << "on_header_field " << data;
    static_cast<ProxyClientPrivate *>(p->data)->lastHeaderField = data;
    return 0;
}

int ProxyClientPrivate::reqOnHeaderValue(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    //qDebug() << "on_header_value " << data;
    static_cast<ProxyClientPrivate *>(p->data)->sendHeader(static_cast<ProxyClientPrivate *>(p->data)->lastHeaderField, data);
    return 0;
}

int ProxyClientPrivate::reqOnUrl(http_parser *p, const char *at, size_t length)
{
    QByteArray data(at, length);
    QString u = QString::fromLatin1(data);
    QUrl url(u);
    if (url.host().isEmpty()) {
        u.prepend("https://");
        url = QUrl(u);
    }
    QByteArray method(http_method_str((enum http_method)p->method));
    qDebug() << "on_url " << method << url;
    static_cast<ProxyClientPrivate *>(p->data)->startQuery(method, url);
    return 0;
}

int ProxyClientPrivate::reqOnBody(http_parser *, const char *, size_t)
{
    return 0;
}


/////////////////////

/*
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
    //qDebug() << "on_body " << a;
    return 0;
}
*/

ProxyClient::ProxyClient(QObject *parent)
    : QTcpSocket(parent)
    , d(std::make_unique<ProxyClientPrivate>(this))
{
    qDebug() << "create " << QThread::currentThread();

    d->socket = new QTcpSocket(this);

    connect(this, &QAbstractSocket::disconnected, this, &ProxyClient::onSrcDisconnected);
    connect(this, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &ProxyClient::onSrcError);
    connect(this, &QIODevice::readyRead, this, &ProxyClient::onSrcReadyRead);

    connect(d->socket, &QAbstractSocket::disconnected, this, &ProxyClient::onDestReadyRead);
    connect(d->socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &ProxyClient::onDestError);
    connect(d->socket, &QIODevice::readyRead, this, &ProxyClient::onDestReadyRead);
}

void ProxyClient::stop()
{
BEGIN_SLOT_WRAPPER
    close();
END_SLOT_WRAPPER
}

ProxyClient::~ProxyClient() = default;

void ProxyClient::onSrcDisconnected()
{
BEGIN_SLOT_WRAPPER
    qDebug() << "SRC disconnected";
    deleteLater();
END_SLOT_WRAPPER
}

void ProxyClient::onSrcError(QAbstractSocket::SocketError socketError)
{
BEGIN_SLOT_WRAPPER
    qDebug() << "SRC socket error" << socketError;
END_SLOT_WRAPPER
}

void ProxyClient::onSrcReadyRead()
{
BEGIN_SLOT_WRAPPER
    QByteArray data = readAll();
    //qDebug() << data;
    if (d->result == ProxyClientPrivate::ConnectQuery) {
        // CONNECT established
        d->socket->write(data);
        return;
    }
    d->parseRequestData(data);
    if (d->result == ProxyClientPrivate::ConnectQuery) {
        qDebug() << "Do SSL connection";
        d->socket->connectToHost(d->host, d->port);
        if (!d->socket->waitForConnected()) {
            // error
            d->sendErrorPage();
            return;
        }
        d->connectionEstablished();
    } else if (d->result == ProxyClientPrivate::NotConnected) {
        // error
        d->sendErrorPage();
        return;
    } else if(d->result == ProxyClientPrivate::ParseError) {
        // parse error
        d->sendErrorPage();
        close();
        return;
    }
END_SLOT_WRAPPER
}

void ProxyClient::onDestDisconnected()
{
BEGIN_SLOT_WRAPPER
    qDebug() << "DEST disconnected";
    // error?
    stop();
END_SLOT_WRAPPER
}

void ProxyClient::onDestError(QAbstractSocket::SocketError socketError)
{
BEGIN_SLOT_WRAPPER
    qDebug() << "DEST socket error" << socketError;
END_SLOT_WRAPPER
}

void ProxyClient::onDestReadyRead()
{
BEGIN_SLOT_WRAPPER
    QByteArray data = d->socket->readAll();
    d->srcSocket->write(data);
    d->srcSocket->waitForBytesWritten();
END_SLOT_WRAPPER
}

}
