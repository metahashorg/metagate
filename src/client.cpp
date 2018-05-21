#include "client.h"

#include <iostream>

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
    LOG << "post message sended" << std::endl;
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
    const std::string requestId = std::to_string(id++);

    callbacks_[requestId] = callback;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, QString::fromStdString(requestId));
    QNetworkReply* reply = manager->get(request);
    CHECK(connect(reply, SIGNAL(finished()), this, SLOT(onTextMessageReceived())), "not connect");
    LOG << "get message sended" << std::endl;
}

void SimpleClient::runCallback(const std::string &id, const std::string &message) {
    const auto foundCallback = callbacks_.find(id);
    CHECK(foundCallback != callbacks_.end(), "not found callback on id " + id);
    const auto callback = std::bind(foundCallback->second, message);
    emit callbackCall(callback);
    callbacks_.erase(foundCallback);
}

void SimpleClient::onTextMessageReceived() {
    try {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

        const std::string requestId = reply->request().attribute(QNetworkRequest::User).toString().toStdString();

        if (reply->error() == QNetworkReply::NoError) {
          QByteArray content = reply->readAll();
          runCallback(requestId, std::string(content.data(), content.size()));
        } else {
            const std::string errorStr = reply->errorString().toStdString();
            LOG << errorStr << std::endl;

            runCallback(requestId, ERROR_BAD_REQUEST);
        }

        reply->deleteLater();
        LOG << "Reply deleted" << std::endl;
    } catch (const Exception &e) {
        LOG << "Error " << e << std::endl;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what() << std::endl;
    } catch (...) {
        LOG << "Unknown error" << std::endl;
    }
}
