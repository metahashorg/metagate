#include "client.h"

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

SimpleClient::SimpleClient() {
    manager = std::make_unique<QNetworkAccessManager>(this);
    qRegisterMetaType<ReturnCallback>("ReturnCallback");
}

void SimpleClient::setParent(QObject *obj) {
    manager->setParent(obj);
}

void SimpleClient::moveToThread(QThread *thread) {
    thread1 = thread;
    QObject::moveToThread(thread);
}

void SimpleClient::startTimer() {
    if (timer == nullptr) {
        timer = new QTimer();
        CHECK(connect(timer, SIGNAL(timeout()), this, SLOT(onTimerEvent())), "not connect timeout");
        if (thread1 != nullptr) {
            CHECK(timer->connect(thread1, SIGNAL(finished()), SLOT(stop())), "not connect finished");
        }
        timer->setInterval(milliseconds(1s).count());
        timer->start();
    }
}

static void addRequestId(QNetworkRequest &request, const std::string &id) {
    request.setAttribute(REQUEST_ID_FIELD, QString::fromStdString(id));
}

static bool isRequestId(QNetworkReply &reply) {
    return reply.request().attribute(REQUEST_ID_FIELD).userType() == QMetaType::QString;
}

static std::string getRequestId(QNetworkReply &reply) {
    CHECK(isRequestId(reply), "Request id field not set");
    return reply.request().attribute(REQUEST_ID_FIELD).toString().toStdString();
}

static void addBeginTime(QNetworkRequest &request, time_point tp) {
    request.setAttribute(TIME_BEGIN_FIELD, QString::fromStdString(std::to_string(timePointToInt(tp))));
}

static bool isBeginTime(QNetworkReply &reply) {
    return reply.request().attribute(TIME_BEGIN_FIELD).userType() == QMetaType::QString;
}

static time_point getBeginTime(QNetworkReply &reply) {
    CHECK(isBeginTime(reply), "begin time field not set");
    const size_t timeBegin = std::stoull(reply.request().attribute(TIME_BEGIN_FIELD).toString().toStdString());
    const time_point timeBeginTp = intToTimePoint(timeBegin);
    return timeBeginTp;
}

static void addTimeout(QNetworkRequest &request, milliseconds timeout) {
    request.setAttribute(TIMOUT_FIELD, QString::fromStdString(std::to_string(timeout.count())));
}

static bool isTimeout(QNetworkReply &reply) {
    return reply.request().attribute(TIMOUT_FIELD).userType() == QMetaType::QString;
}

static milliseconds getTimeout(QNetworkReply &reply) {
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

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout) {
    const std::string requestId = std::to_string(id++);

    startTimer();

    callbacks_[requestId] = callback;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    addRequestId(request, requestId);
    if (isTimeout) {
        const time_point time = ::now();
        addBeginTime(request, time);
        addTimeout(request, timeout);
    }
    QNetworkReply* reply = manager->post(request, message.toUtf8());
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    //LOG << "post message sended";
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback) {
    sendMessagePost(url, message, callback, false, milliseconds(0));
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout) {
    sendMessagePost(url, message, callback, true, timeout);
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
    const std::string requestId = std::to_string(id++);

    startTimer();

    callbacks_[requestId] = callback;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    addRequestId(request, requestId);
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    LOG << "get message sended";
}

void SimpleClient::ping(const QString &address, const PingCallback &callback, milliseconds timeout) {
    const std::string requestId = std::to_string(id++);

    startTimer();

    pingCallbacks_[requestId] = std::bind(callback, address, _1, _2);
    QNetworkRequest request(address);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    const time_point time = ::now();
    addRequestId(request, requestId);
    addBeginTime(request, time);
    addTimeout(request, timeout);
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onPingReceived()), Qt::QueuedConnection), "not connect");

    requests[requestId] = reply;
    //LOG << "ping message sended ";
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
        QByteArray content = reply->readAll();
        runCallback(callbacks_, requestId, std::string(content.data(), content.size()), TypedException());
    } else {
        const std::string error = reply->errorString().toStdString() + ". " + QString(reply->readAll()).toStdString();
        //LOG << error;

        runCallback(callbacks_, requestId, "", TypedException(TypeErrors::CLIENT_ERROR, error));
    }

    reply->deleteLater();
END_SLOT_WRAPPER
}
