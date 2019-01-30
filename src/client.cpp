#include "client.h"

#include <iostream>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"
#include "QRegister.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>

QT_USE_NAMESPACE

const static QNetworkRequest::Attribute REQUEST_ID_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 0);
const static QNetworkRequest::Attribute TIME_BEGIN_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 1);
const static QNetworkRequest::Attribute TIMOUT_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 2);

const int SimpleClient::ServerException::BAD_REQUEST_ERROR = QNetworkReply::ProtocolInvalidOperationError;

SimpleClient::SimpleClient() {
    manager = std::make_unique<QNetworkAccessManager>(this);
    Q_REG(SimpleClient::ReturnCallback, "SimpleClient::ReturnCallback");
}

void SimpleClient::setParent(QObject *obj) {
    manager->setParent(obj);
}

void SimpleClient::moveToThread(QThread *thread) {
    thread1 = thread;
    QObject::moveToThread(thread);
}

void SimpleClient::startTimer1() {
    if (timer == nullptr) {
        timer = new QTimer();
        CHECK(connect(timer, &QTimer::timeout, this, &SimpleClient::onTimerEvent), "not connect timeout");
        if (thread1 != nullptr) {
            CHECK(connect(thread1, &QThread::finished, timer, &QTimer::stop), "not connect finished");
        }
        timer->setInterval(milliseconds(1s).count());
        timer->start();
    }
}

static void addRequestId(QNetworkRequest &request, const std::string &id) {
    request.setAttribute(REQUEST_ID_FIELD, QString::fromStdString(id));
}

static bool isRequestId(const QNetworkReply &reply) {
    return reply.request().attribute(REQUEST_ID_FIELD).userType() == QMetaType::QString;
}

static std::string getRequestId(const QNetworkReply &reply) {
    CHECK(isRequestId(reply), "Request id field not set");
    return reply.request().attribute(REQUEST_ID_FIELD).toString().toStdString();
}

static void addBeginTime(QNetworkRequest &request, time_point tp) {
    request.setAttribute(TIME_BEGIN_FIELD, QString::fromStdString(std::to_string(timePointToInt(tp))));
}

static bool isBeginTime(const QNetworkReply &reply) {
    return reply.request().attribute(TIME_BEGIN_FIELD).userType() == QMetaType::QString;
}

static time_point getBeginTime(const QNetworkReply &reply) {
    CHECK(isBeginTime(reply), "begin time field not set");
    const size_t timeBegin = std::stoull(reply.request().attribute(TIME_BEGIN_FIELD).toString().toStdString());
    const time_point timeBeginTp = intToTimePoint(timeBegin);
    return timeBeginTp;
}

static void addTimeout(QNetworkRequest &request, milliseconds timeout) {
    request.setAttribute(TIMOUT_FIELD, QString::fromStdString(std::to_string(timeout.count())));
}

static bool isTimeout(const QNetworkReply &reply) {
    return reply.request().attribute(TIMOUT_FIELD).userType() == QMetaType::QString;
}

static milliseconds getTimeout(const QNetworkReply &reply) {
    CHECK(isTimeout(reply), "Timeout field not set");
    return milliseconds(std::stol(reply.request().attribute(TIMOUT_FIELD).toString().toStdString()));
}

void SimpleClient::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    std::vector<std::reference_wrapper<QNetworkReply>> toDelete;
    const time_point timeEnd = ::now();
    for (auto &iter: requests) {
        auto &reply = *iter.second;

        if (isTimeout(reply)) {
            const milliseconds timeout = getTimeout(reply);
            const time_point timeBegin = getBeginTime(reply);
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);
            if (duration >= timeout) {
                LOG << "Timeout request";
                toDelete.emplace_back(reply);
            }
        }
    }

    for (QNetworkReply& reply: toDelete) {
        reply.abort();
    }
END_SLOT_WRAPPER
}

template<typename Callback>
void SimpleClient::sendMessageInternal(
    bool isPost,
    std::unordered_map<std::string, Callback> &callbacks,
    const QUrl &url,
    const QString &message,
    const Callback &callback,
    bool isTimeout,
    milliseconds timeout,
    bool isClearCache,
    TextMessageReceived onTextMessageReceived,
    bool isQueuedConnection
) {
    const std::string requestId = std::to_string(id++);

    startTimer1();

    callbacks[requestId] = callback;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    addRequestId(request, requestId);
    if (isTimeout) {
        const time_point time = ::now();
        addBeginTime(request, time);
        addTimeout(request, timeout);
    }
    if (isClearCache) {
        manager->clearAccessCache();
        manager->clearConnectionCache();
    }
    QNetworkReply* reply;
    if (isPost) {
        reply = manager->post(request, message.toUtf8());
    } else {
        reply = manager->get(request);
    }
    Qt::ConnectionType connType = Qt::AutoConnection;
    if (isQueuedConnection) {
        connType = Qt::QueuedConnection;
    }
    CHECK(connect(reply, &QNetworkReply::finished, this, onTextMessageReceived, connType), "not connect onTextMessageReceived");
    requests[requestId] = reply;
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout, bool isClearCache) {
    sendMessageInternal(true, callbacks_, url, message, callback, isTimeout, timeout, isClearCache, &SimpleClient::onTextMessageReceived, false);
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback) {
    sendMessagePost(url, message, callback, false, milliseconds(0), false);
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout, bool isClearCache) {
    sendMessagePost(url, message, callback, true, timeout, isClearCache);
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback, bool isTimeout, milliseconds timeout) {
    sendMessageInternal(false, callbacks_, url, "", callback, isTimeout, timeout, false, &SimpleClient::onTextMessageReceived, false);
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
    sendMessageGet(url, callback, false, milliseconds(0));
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback, milliseconds timeout) {
    sendMessageGet(url, callback, true, timeout);
}

void SimpleClient::ping(const QString &address, const PingCallback &callback, milliseconds timeout) {
    sendMessageInternal(false, pingCallbacks_, address, "", PingCallbackInternal(std::bind(callback, address, _1, _2)), true, timeout, false, &SimpleClient::onPingReceived, true);
}

template<class Callbacks, typename... Message>
void SimpleClient::runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages) {
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + id);
    const auto callback = std::bind(foundCallback->second, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
    requests.erase(id);
}

void SimpleClient::onPingReceived() {
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    const std::string requestId = getRequestId(*reply);
    const time_point timeBegin = getBeginTime(*reply);
    const time_point timeEnd = ::now();
    const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);

    std::string response;
    if (reply->isReadable()) {
        QByteArray content = reply->readAll();
        response = std::string(content.data(), content.size());
    }

    runCallback(pingCallbacks_, requestId, duration, response);

    reply->deleteLater();
END_SLOT_WRAPPER
}

void SimpleClient::onTextMessageReceived() {
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    const std::string requestId = getRequestId(*reply);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray content;
        if (reply->isReadable()) {
            content = reply->readAll();
        }
        runCallback(callbacks_, requestId, std::string(content.data(), content.size()), ServerException());
    } else {
        std::string errorStr;
        if (reply->isReadable()) {
            errorStr = QString(reply->readAll()).toStdString();
        }
        //LOG << error;

        runCallback(callbacks_, requestId, "", ServerException(reply->error(), reply->errorString().toStdString(), errorStr));
    }

    reply->deleteLater();
END_SLOT_WRAPPER
}
