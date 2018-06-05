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

SimpleClient::SimpleClient() {
    manager = std::make_unique<QNetworkAccessManager>(this);
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
        timer = new QTimer(this);
        CHECK(connect(timer, SIGNAL(timeout()), this, SLOT(onTimerEvent())), "not connect timeout");
        if (thread1 != nullptr) {
            CHECK(timer->connect(thread1, SIGNAL(finished()), SLOT(stop())), "not connect finished");
        }
        timer->setInterval(milliseconds(1s).count());
        timer->start();
    }
}

const std::string SimpleClient::ERROR_BAD_REQUEST = "Error bad request";

void SimpleClient::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    const time_point timeEnd = ::now();
    for (auto &iter: requests) {
        auto *reply = iter.second;

        if (reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).userType() == QMetaType::QString) {
            const milliseconds timeout(std::stol(reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString().toStdString()));
            const size_t timeBegin = std::stoull(reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString().toStdString());
            const time_point timeBeginTp = intToTimePoint(timeBegin);
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBeginTp);
            if (duration >= timeout) {
                LOG << "Timeout request";
                reply->abort();
            }
        }
    }
END_SLOT_WRAPPER
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback) {
    const std::string requestId = std::to_string(id++);

    startTimer();

    callbacks_[requestId] = callback;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    QNetworkReply* reply = manager->post(request, message.toUtf8());
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    LOG << "post message sended";
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
    const std::string requestId = std::to_string(id++);

    startTimer();

    callbacks_[requestId] = callback;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    LOG << "get message sended";
}

void SimpleClient::ping(const QString &address, const PingCallback &callback, milliseconds timeout) {
    const std::string requestId = std::to_string(id++);

    startTimer();

    pingCallbacks_[requestId] = std::bind(callback, address, _1);
    QNetworkRequest request("http://" + address);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    const time_point time = ::now();
    const size_t timeBegin = timePointToInt(time);
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    request.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QString::fromStdString(std::to_string(timeBegin)));
    request.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QString::fromStdString(std::to_string(timeout.count())));
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onPingReceived()), Qt::QueuedConnection), "not connect");

    requests[requestId] = reply;
    //LOG << "ping message sended ";
}

template<class Callbacks, typename Message>
void SimpleClient::runCallback(Callbacks &callbacks, const std::string &id, const Message &message) {
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + id);
    const auto callback = std::bind(foundCallback->second, message);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
    requests.erase(id);
}

void SimpleClient::onPingReceived() {
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    const std::string requestId = reply->request().attribute(QNetworkRequest::User).toString().toStdString();
    const size_t timeBegin = std::stoull(reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString().toStdString());
    const time_point timeEnd = ::now();
    const time_point timeBeginTp = intToTimePoint(timeBegin);
    const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBeginTp);

    runCallback(pingCallbacks_, requestId, duration);

    reply->deleteLater();
END_SLOT_WRAPPER
}

void SimpleClient::onTextMessageReceived() {
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    const std::string requestId = reply->request().attribute(QNetworkRequest::User).toString().toStdString();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray content = reply->readAll();
        runCallback(callbacks_, requestId, std::string(content.data(), content.size()));
    } else {
        const std::string errorStr = reply->errorString().toStdString();
        LOG << errorStr;

        runCallback(callbacks_, requestId, ERROR_BAD_REQUEST);
    }

    reply->deleteLater();
END_SLOT_WRAPPER
}
