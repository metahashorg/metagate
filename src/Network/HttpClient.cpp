#include "HttpClient.h"

#include <iostream>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

#include <QThread>

QT_USE_NAMESPACE

HttpSimpleClient::HttpSimpleClient() {
    Q_REG(HttpSimpleClient::ReturnCallback, "HttpSimpleClient::ReturnCallback");
}

void HttpSimpleClient::moveToThread(QThread *thread)
{
    thread1 = thread;
    QObject::moveToThread(thread);
}

void HttpSimpleClient::startTimer1()
{
    if (timer == nullptr) {
        timer = new QTimer();
        Q_CONNECT(timer, &QTimer::timeout, this, &HttpSimpleClient::onTimerEvent);
        if (thread1 != nullptr) {
            Q_CONNECT(thread1, &QThread::finished, timer, &QTimer::stop);
        }
        timer->setInterval(milliseconds(1s).count());
        timer->start();
    }
}

static void addRequestId(AbstractSocket *socket, const int id)
{
    socket->setRequestId(id);
}

static int getRequestId(AbstractSocket *socket)
{
    return socket->requestId();
}

static void addBeginTime(AbstractSocket *socket, time_point tp)
{
    socket->setTimePoint(tp);
}

static time_point getBeginTime(AbstractSocket *socket)
{
    return socket->timePoint();
}

static void addTimeout(AbstractSocket *socket, milliseconds timeout)
{
    socket->setTimeOut(timeout);
}

static milliseconds getTimeout(AbstractSocket *socket)
{
    return  socket->timeOut();
}

void HttpSimpleClient::onTimerEvent()
{
BEGIN_SLOT_WRAPPER
    std::vector<AbstractSocket *> toStop;
    const time_point timeEnd = ::now();
    for (auto &iter: sockets) {
        AbstractSocket *socket = iter.second;
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

    for (AbstractSocket *socket: toStop) {
        socket->stop();
    }
    END_SLOT_WRAPPER
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout)
{
    startTimer1();

    std::unique_ptr<HttpSocket> socket(new HttpSocket(url, message));
    callbacks[id] = callback;
    sockets[id] = socket.get();
    addRequestId(socket.get(), id);
    id++;
    if (isTimeout) {
        const time_point time = ::now();
        addBeginTime(socket.get(), time);
        addTimeout(socket.get(), timeout);
    }
    Q_CONNECT(socket.get(), &HttpSocket::finished, this, &HttpSimpleClient::onSocketFinished);
    socket.release()->start();
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback)
{
    sendMessagePost(url, message, callback, false, milliseconds(0));
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout) {
    sendMessagePost(url, message, callback, true, timeout);
}

void HttpSimpleClient::sendMessagePing(const QUrl &url, const ClientCallback &callback, milliseconds timeout) {
    startTimer1();

    std::unique_ptr<PingSocket> socket(new PingSocket(url));
    callbacks[id] = callback;
    sockets[id] = socket.get();
    addRequestId(socket.get(), id);
    id++;
    const time_point time = ::now();
    addBeginTime(socket.get(), time);
    addTimeout(socket.get(), timeout);
    Q_CONNECT(socket.get(), &PingSocket::finished, this, &HttpSimpleClient::onSocketFinished);
    socket.release()->start();
}

template<class Callbacks, typename... Message>
void HttpSimpleClient::runCallback(Callbacks &callbacks, const int id, Message&&... messages)
{
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + std::to_string(id));
    const auto callback = std::bind(foundCallback->second, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
    sockets.erase(id);
}


void HttpSimpleClient::onSocketFinished()
{
BEGIN_SLOT_WRAPPER
    AbstractSocket *socket = qobject_cast<AbstractSocket *>(sender());
    CHECK(socket, "Not socket object");
    const int requestId = getRequestId(socket);
    if (socket->hasError()) {
        runCallback(callbacks, requestId, "", TypedException(TypeErrors::CLIENT_ERROR, std::to_string(socket->errorC()) + " " + socket->errorString().toStdString()));
    } else {
        QByteArray content = socket->getReply();
        runCallback(callbacks, requestId, std::string(content.data(), content.size()), TypedException());
    }

    socket->deleteLater();

END_SLOT_WRAPPER
}

void AbstractSocket::stop()
{
    abort();
    m_error = true;
    emit finished();
}

int AbstractSocket::requestId() const
{
    return m_requestId;
}

void AbstractSocket::setRequestId(const int s)
{
    m_requestId = s;
}

bool AbstractSocket::hasTimeOut() const
{
    return m_hasTimeOut;
}

bool AbstractSocket::hasError() const
{
    return m_error;
}

time_point AbstractSocket::timePoint() const
{
    return m_timePoint;
}

void AbstractSocket::setTimePoint(time_point s)
{
    m_timePoint = s;
    m_hasTimeOut = true;
}

milliseconds AbstractSocket::timeOut() const
{
    return m_timeOut;
}

void AbstractSocket::setTimeOut(milliseconds s)
{
    m_timeOut = s;
}

QByteArray AbstractSocket::getReply() const
{
    return m_reply;
}

void HttpSocket::onConnected()
{
BEGIN_SLOT_WRAPPER
    QByteArray d = getHttpPostHeader();
    d += m_message.toLatin1();
    write(d);
END_SLOT_WRAPPER
}

void HttpSocket::onError(QAbstractSocket::SocketError socketError)
{
BEGIN_SLOT_WRAPPER
    errorCode = socketError;
    m_error = true;
    emit finished();
END_SLOT_WRAPPER
}

void HttpSocket::onReadyRead()
{
BEGIN_SLOT_WRAPPER
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
END_SLOT_WRAPPER
}

HttpSocket::HttpSocket(const QUrl &url, const QString &message, QObject *parent)
    : AbstractSocket(parent)
    , m_url(url)
    , m_message(message)
{
    Q_CONNECT(this, &QAbstractSocket::connected, this, &HttpSocket::onConnected);
    Q_CONNECT(this, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &HttpSocket::onError);
    Q_CONNECT(this, &QIODevice::readyRead, this, &HttpSocket::onReadyRead);
}

void HttpSocket::start()
{
    connectToHost(m_url.host(), m_url.port(80));
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

PingSocket::PingSocket(const QUrl &url, QObject *parent)
    : AbstractSocket(parent)
    , m_url(url)
{
    Q_CONNECT(this, &QAbstractSocket::connected, this, &PingSocket::onConnected);
    Q_CONNECT(this, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &PingSocket::onError);
    Q_CONNECT(this, &QIODevice::readyRead, this, &PingSocket::onReadyRead);
}

void PingSocket::start() {
    connectToHost(m_url.host(), m_url.port(80));
}

void PingSocket::onConnected() {
BEGIN_SLOT_WRAPPER
    write("GET / HTTP/1.1\r\n\r\n");
END_SLOT_WRAPPER
}

void PingSocket::onError(QAbstractSocket::SocketError socketError) {
BEGIN_SLOT_WRAPPER
    errorCode = socketError;
    m_error = true;
    if (!isEmited) {
        emit finished();
    }
END_SLOT_WRAPPER
}

void PingSocket::onReadyRead() {
BEGIN_SLOT_WRAPPER
    QByteArray d = readAll();
    m_data += d;
    isEmited = true;
    emit finished();
END_SLOT_WRAPPER
}
