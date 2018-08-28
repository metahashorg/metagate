#include "httpclient.h"

#include <iostream>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>

QT_USE_NAMESPACE

const static QNetworkRequest::Attribute REQUEST_ID_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 0);
const static QNetworkRequest::Attribute TIME_BEGIN_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 1);
const static QNetworkRequest::Attribute TIMOUT_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 2);

HttpSimpleClient::HttpSimpleClient() {
    //manager = std::make_unique<QNetworkAccessManager>(this);
}

void HttpSimpleClient::setParent(QObject *obj) {
    Q_UNUSED(obj);
}

void HttpSimpleClient::moveToThread(QThread *thread) {
    thread1 = thread;
    QObject::moveToThread(thread);
}

void HttpSimpleClient::startTimer() {
    /*if (timer == nullptr) {
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
    }*/
}

static void addRequestId(HttpSocket *socket, const std::string &id)
{
    socket->setRequestId(id);
    //request.setAttribute(REQUEST_ID_FIELD, QString::fromStdString(id));
}

//static bool isRequestId(QNetworkReply &reply) {
//    return reply.request().attribute(REQUEST_ID_FIELD).userType() == QMetaType::QString;
//}

static std::string getRequestId(HttpSocket *socket)
{
    //CHECK(isRequestId(reply), "Request id field not set");
    return socket->requestId();
    //return reply.request().attribute(REQUEST_ID_FIELD).toString().toStdString();
}

static void addBeginTime(HttpSocket *socket, time_point tp)
{
    socket->setTimePoint(tp);
    //request.setAttribute(TIME_BEGIN_FIELD, QString::fromStdString(std::to_string(timePointToInt(tp))));
}

//static bool isBeginTime(QNetworkReply &reply) {
//    return reply.request().attribute(TIME_BEGIN_FIELD).userType() == QMetaType::QString;
//}

static time_point getBeginTime(HttpSocket *socket)
{
//    CHECK(isBeginTime(reply), "begin time field not set");
//    const size_t timeBegin = std::stoull(reply.request().attribute(TIME_BEGIN_FIELD).toString().toStdString());
//    const time_point timeBeginTp = intToTimePoint(timeBegin);
//    return timeBeginTp;
    return socket->timePoint();
}

static void addTimeout(HttpSocket *socket, milliseconds timeout)
{
    socket->setTimeOut(timeout);
    //request.setAttribute(TIMOUT_FIELD, QString::fromStdString(std::to_string(timeout.count())));
}

//static bool isTimeout(QNetworkReply &reply) {
//    return reply.request().attribute(TIMOUT_FIELD).userType() == QMetaType::QString;
//}

static milliseconds getTimeout(HttpSocket *socket)
{
    return  socket->timeOut();
    //CHECK(isTimeout(reply), "Timeout field not set");
    //return milliseconds(std::stol(reply.request().attribute(TIMOUT_FIELD).toString().toStdString()));
}

const std::string HttpSimpleClient::ERROR_BAD_REQUEST = "Error bad request";

void HttpSimpleClient::onTimerEvent()
{
BEGIN_SLOT_WRAPPER
     std::vector<std::reference_wrapper<HttpSocket *>> toDelete;
    const time_point timeEnd = ::now();
    for (auto &iter: sockets) {
        HttpSocket *socket = iter.second;

        if (socket->hasTimeOut()) {
            const milliseconds timeout = getTimeout(socket);
            const time_point timeBegin = getBeginTime(socket);
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);
            if (duration >= timeout) {
                LOG << "Timeout request";
                toDelete.emplace_back(socket);
            }
        }
    }

    for (HttpSocket *socket: toDelete) {
        //reply.abort();
        delete socket;
    }
    END_SLOT_WRAPPER
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout)
{
    const std::string requestId = std::to_string(id++);
    //startTimer();

    HttpSocket *socket = new HttpSocket(url, message);
    callbacks_[requestId] = callback;
    //QNetworkRequest request(url);
    //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    addRequestId(socket, requestId);
    if (isTimeout) {
        const time_point time = ::now();
        addBeginTime(socket, time);
        addTimeout(socket, timeout);
    }

    connect(socket, &HttpSocket::finished, this, &HttpSimpleClient::onSocketFinished);
    socket->start();
    //LOG << "post message sended";
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback)
{
    sendMessagePost(url, message, callback, false, milliseconds(0));
}

void HttpSimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout) {
    sendMessagePost(url, message, callback, true, timeout);
}

//void HttpSimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
//    const std::string requestId = std::to_string(id++);

//    startTimer();

//    callbacks_[requestId] = callback;
//    QNetworkRequest request(url);
//    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//    addRequestId(request, requestId);
//    QNetworkReply* reply = manager->get(request);
//    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
//    LOG << "get message sended";

//}

//void HttpSimpleClient::ping(const QString &address, const PingCallback &callback, milliseconds timeout) {
//    const std::string requestId = std::to_string(id++);

//    startTimer();

//    pingCallbacks_[requestId] = std::bind(callback, address, _1, _2);
//    QNetworkRequest request("http://" + address);
//    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//    const time_point time = ::now();
//    addRequestId(request, requestId);
//    addBeginTime(request, time);
//    addTimeout(request, timeout);
//    QNetworkReply* reply = manager->get(request);
//    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onPingReceived()), Qt::QueuedConnection), "not connect");

//    requests[requestId] = reply;
//    //LOG << "ping message sended ";
//}

template<class Callbacks, typename... Message>
void HttpSimpleClient::runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages)
{
    qDebug() << "run";
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + id);
    const auto callback = std::bind(foundCallback->second, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
    sockets.erase(id);
}

//void HttpSimpleClient::onPingReceived() {
//BEGIN_SLOT_WRAPPER
//    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

//    const std::string requestId = getRequestId(*reply);
//    const time_point timeBegin = getBeginTime(*reply);
//    const time_point timeEnd = ::now();
//    const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);

//    std::string response;
//    if (reply->isReadable()) {
//        QByteArray content = reply->readAll();
//        response = std::string(content.data(), content.size());
//    }

//    runCallback(pingCallbacks_, requestId, duration, response);

//    reply->deleteLater();
//END_SLOT_WRAPPER
//}


void HttpSimpleClient::onSocketFinished()
{
    BEGIN_SLOT_WRAPPER
        HttpSocket *socket = qobject_cast<HttpSocket *>(sender());

        const std::string requestId = getRequestId(socket);

        //if (reply->error() == QNetworkReply::NoError) {
            QByteArray content = socket->getReply();
            qDebug() << "finished " << content;
            runCallback(callbacks_, requestId, std::string(content.data(), content.size()));
        /*} else {
            const std::string errorStr = reply->errorString().toStdString();
            LOG << errorStr;

            runCallback(callbacks_, requestId, ERROR_BAD_REQUEST);
        }*/

        socket->deleteLater();

    END_SLOT_WRAPPER
}

HttpSocket::HttpSocket(const QUrl &url, const QString &message, QObject *parent)
    : QTcpSocket(parent)
    , m_url(url)
    , m_message(message)
{
    connect(this, &QAbstractSocket::connected, this, &HttpSocket::onConnected);
    connect(this, &QIODevice::readyRead, this, &HttpSocket::onReadyRead);
}

HttpSocket::~HttpSocket()
{
    qDebug() << "remove";
}

void HttpSocket::start()
{
    qDebug() << "start";
    connectToHost(m_url.host(), m_url.port(80));
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

void HttpSocket::onReadyRead()
{
    QByteArray d = readAll();
    qDebug() << d;
    m_data += d;
    parseResponseHeader();
    if (m_headerParsed) {
        if (m_contentLength != -1) {
            if (m_data.length() >= m_contentLength) {
                m_reply = m_data.left(m_contentLength);
                qDebug() << "! " << m_reply;
                emit finished();
            }
        } else {

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
        qDebug() << s;
        m_data = m_data.mid(index + 1);
        if (s.isEmpty() || s == QByteArray("\r")) {
            qDebug() << "HEADER PARSED";
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
