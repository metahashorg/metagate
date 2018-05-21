#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>

#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

using ClientCallback = std::function<void(const std::string &response)>;

using ReturnCallback = std::function<void()>;

/*
   На каждый поток должен быть один экземпляр класса.
   */
class SimpleClient : public QObject
{
    Q_OBJECT
public:

    static const std::string ERROR_BAD_REQUEST;

public:

    explicit SimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessageGet(const QUrl &url, const ClientCallback &callback);

    void setParent(QObject *obj);

Q_SIGNALS:

    void callbackCall(ReturnCallback callback);

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onTextMessageReceived();

private:

    void runCallback(const std::string &id, const std::string &message);

private:
    std::unique_ptr<QNetworkAccessManager> manager;
    std::unordered_map<std::string, ClientCallback> callbacks_;

    int id = 0;
};

#endif // CLIENT_H
