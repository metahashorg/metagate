#include "client.h"

#include <iostream>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"

#include <QNetworkRequest>
#include <QNetworkReply>

QT_USE_NAMESPACE

SimpleClient::SimpleClient() {
    manager = std::make_unique<QNetworkAccessManager>(this);
}

void SimpleClient::setParent(QObject *obj) {
    manager->setParent(obj);
}

const std::string SimpleClient::ERROR_BAD_REQUEST = "Error bad request";

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback) {
    const std::string requestId = std::to_string(id++);

    callbacks_[requestId] = callback;
    //std::cout << "send message " << requestId << " " << message.toStdString() << std::endl;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    QNetworkReply* reply = manager->post(request, message.toUtf8());
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    LOG << "post message sended";
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
    const std::string requestId = std::to_string(id++);

    callbacks_[requestId] = callback;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    LOG << "get message sended";
}

void SimpleClient::ping(const QString &address, const PingCallback &callback) {
    const std::string requestId = std::to_string(id++);

    pingCallbacks_[requestId] = std::bind(callback, address, _1);
    QNetworkRequest request("http://" + address);
    const time_point time = ::now();
    const size_t timeBegin = timePointToInt(time);
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    request.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QString::fromStdString(std::to_string(timeBegin)));
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onPingReceived()), Qt::QueuedConnection), "not connect");
    LOG << "ping message sended ";
}

template<class Callbacks, typename Message>
void SimpleClient::runCallback(Callbacks &callbacks, const std::string &id, const Message &message) {
    const auto foundCallback = callbacks.find(id);
    CHECK(foundCallback != callbacks.end(), "not found callback on id " + id);
    const auto callback = std::bind(foundCallback->second, message);
    emit callbackCall(callback);
    callbacks.erase(foundCallback);
}

void SimpleClient::onPingReceived() {
    try {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

        const std::string requestId = reply->request().attribute(QNetworkRequest::User).toString().toStdString();
        const size_t timeBegin = std::stoull(reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString().toStdString());
        const time_point timeEnd = ::now();
        const time_point timeBeginTp = intToTimePoint(timeBegin);
        const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBeginTp);

        runCallback(pingCallbacks_, requestId, duration);

        reply->deleteLater();
        LOG << "Reply deleted";
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
}

void SimpleClient::onTextMessageReceived() {
    try {
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
        LOG << "Reply deleted";
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
}
