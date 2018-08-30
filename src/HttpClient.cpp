#include "HttpClient.h"

#include <iostream>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>

QT_USE_NAMESPACE

HttpSimpleClient::HttpSimpleClient() {
    qRegisterMetaType<ReturnCallback>("ReturnCallback");
}

void HttpSimpleClient::moveToThread(QThread *thread)
{
    thread1 = thread;
    QObject::moveToThread(thread);
}

void HttpSimpleClient::startTimer()
{
    if (timer == nullptr) {
        timer = new QTimer();
        CHECK(connect(timer, SIGNAL(timeout()), this, SLOT(onTimerEvent())), "not connect timeout");
        if (thread1 != nullptr) {
            timer->setParent(thread1);
            CHECK(timer->connect(thread1, SIGNAL(finished()), SLOT(stop())), "not connect finished");
        } else {
            timer->setParent(this);
        }
        timer->setInterval(milliseconds(1s).count());
        timer->start();
    }
}

static void addRequestId(HttpSocket *socket, const std::string &id)
{
    socket->setRequestId(id);
}

static std::string getRequestId(HttpSocket *socket)
{
    return socket->requestId();
}

static void addBeginTime(HttpSocket *socket, time_point tp)
{
    socket->setTimePoint(tp);
}

static time_point getBeginTime(HttpSocket *socket)
{
    return socket->timePoint();
}

static void addTimeout(HttpSocket *socket, milliseconds timeout)
{
    socket->setTimeOut(timeout);
}

static milliseconds getTimeout(HttpSocket *socket)
{
    return  socket->timeOut();
}

void HttpSimpleClient::onTimerEvent()
{
BEGIN_SLOT_WRAPPER
    std::vector<HttpSocket *> toStop;
    const time_point timeEnd = ::now();
    for (auto &iter: sockets) {
        HttpSocket *socket = iter.second;

        if (socket->hasTimeOut()) {
            const milliseconds timeout = getTimeout(socket);
            const time_point timeBegin = getBeginTime(socket);
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);
            if (duration >= timeout) {
                LOG << "Timeout request";
                toStop.push_back(socket);
            }
        }
    }

    for (HttpSocket *socket: toStop) {
        socket->stop();
    }
    END_SLOT_WRAPPER
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout)
{
    const std::string requestId = std::to_string(id++);
    startTimer();

    std::unique_ptr<HttpSocket> socket(new HttpSocket(url, message));
    callbacks_[requestId] = callback;
    addRequestId(socket.get(), requestId);
    if (isTimeout) {
        const time_point time = ::now();
        addBeginTime(socket.get(), time);
        addTimeout(socket.get(), timeout);
    }

    connect(socket.get(), &HttpSocket::finished, this, &HttpSimpleClient::onSocketFinished);
    socket.release()->start();
    LOG << "post message sended";
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback)
{
    sendMessagePost(url, message, callback, false, milliseconds(0));
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout) {
    sendMessagePost(url, message, callback, true, timeout);
}

template<class Callbacks, typename... Message>
void HttpSimpleClient::runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages)
{
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + id);
    const auto callback = std::bind(foundCallback->second, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
    sockets.erase(id);
}


void HttpSimpleClient::onSocketFinished()
{
BEGIN_SLOT_WRAPPER
    HttpSocket *socket = qobject_cast<HttpSocket *>(sender());
    CHECK(socket, "Not socket object");

    const std::string requestId = getRequestId(socket);

    if (socket->hasError()) {
        runCallback(callbacks_, requestId, "", TypedException(TypeErrors::CLIENT_ERROR, "error"));
    } else {
        QByteArray content = socket->getReply();
        runCallback(callbacks_, requestId, std::string(content.data(), content.size()), TypedException());
    }

    socket->deleteLater();

END_SLOT_WRAPPER
}

HttpSocket::HttpSocket(const QUrl &url, const QString &message, QObject *parent)
    : QTcpSocket(parent)
    , m_url(url)
    , m_message(message)
{
    connect(this, &QAbstractSocket::connected, this, &HttpSocket::onConnected);
    connect(this, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &HttpSocket::onError);
    connect(this, &QIODevice::readyRead, this, &HttpSocket::onReadyRead);
}

void HttpSocket::start()
{
    connectToHost(m_url.host(), m_url.port(80));
}

void HttpSocket::stop()
{
    abort();
    m_error = true;
    emit finished();
}

std::string HttpSocket::requestId() const
{
    return  m_requestId;
}

void HttpSocket::setRequestId(const std::string &s)
{
    m_requestId = s;
}

bool HttpSocket::hasTimeOut() const
{
    return m_hasTimeOut;
}

bool HttpSocket::hasError() const
{
    return m_error;
}

time_point HttpSocket::timePoint() const
{
    return m_timePoint;
}

void HttpSocket::setTimePoint(time_point s)
{
    m_timePoint = s;
    m_hasTimeOut = true;
}

milliseconds HttpSocket::timeOut() const
{
    return m_timeOut;
}

void HttpSocket::setTimeOut(milliseconds s)
{
    m_timeOut = s;
}

QByteArray HttpSocket::getReply() const
{
    return m_reply;
}

void HttpSocket::onConnected()
{
    QByteArray d = getHttpPostHeader();
    d += m_message.toLatin1();
    write(d);
}

void HttpSocket::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    m_error = true;
    emit finished();
}

void HttpSocket::onReadyRead()
{
    QByteArray d = readAll();
    m_data += d;
    parseResponseHeader();
    if (m_error) {
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
}

QByteArray HttpSocket::getHttpPostHeader() const
{
    QString data;

    data += QStringLiteral("POST / HTTP/1.1\r\n");
    data += QStringLiteral("Host: ") + m_url.host() + QStringLiteral(":") + QString::number(m_url.port(80)) + QStringLiteral("\r\n");
    data += QStringLiteral("Content-Type: application/x-www-form-urlencoded\r\n");
    data += QStringLiteral("Accept: */*\r\n");
    data += QStringLiteral("Accept-Encoding: identity\r\n");
    data += QStringLiteral("Content-Length: %1\r\n").arg(m_message.toLatin1().length());
    data += QStringLiteral("\r\n");

    return data.toLatin1();
}

void HttpSocket::parseResponseHeader()
{
    if (m_headerParsed)
        return;
    while(true) {
        int index = m_data.indexOf('\n');
        if (index == -1)
            break;
        QByteArray s = m_data.left(index);
        if (!m_firstHeaderStringParsed) {
            if (!s.startsWith("HTTP/1.1 200 OK") && !s.startsWith("HTTP/1.0 200 OK")) {
                // HTTP error
                m_error = true;
                return;
            }
            m_firstHeaderStringParsed = true;
        }
        m_data = m_data.mid(index + 1);
        if (s.isEmpty() || s == QByteArray("\r")) {
            m_headerParsed = true;
            return;
        }
        if (s.startsWith("Content-Length: ")) {
            s = s.mid(16);
            if (s.endsWith('\r'))
                s = s.left(s.length() - 1);
            m_contentLength = s.toInt();
        }
    }
}
