#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QTcpSocket>

#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

#include "duration.h"

using ClientCallback = std::function<void(const std::string &response)>;

//using PingCallback = std::function<void(const QString &address, const milliseconds &time, const std::string &response)>;

using ReturnCallback = std::function<void()>;

class HttpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit HttpSocket(const QUrl &url, const QString &message, QObject *parent = nullptr);
    virtual ~HttpSocket();

    void start();

    std::string requestId() const;
    void setRequestId(const std::string &s);

    bool hasTimeOut() const;

    time_point timePoint() const;
    void setTimePoint(time_point s);

    milliseconds timeOut() const;
    void setTimeOut(milliseconds s);

    QByteArray getReply() const;

signals:
    void finished();

private slots:
    void onConnected();
    void onReadyRead();
private:
    QByteArray getHttpPostHeader() const;
    void parseResponseHeader();

    std::string m_requestId;
    time_point m_timePoint;
    milliseconds m_timeOut;
    bool m_hasTimeOut = false;

    QUrl m_url;
    QString m_message;
    QByteArray m_data;
    bool m_headerParsed = false;
    int m_contentLength = -1;
    QByteArray m_reply;
};

/*
   На каждый поток должен быть один экземпляр класса.
   */
class HttpSimpleClient : public QObject
{
    Q_OBJECT

//private:

    //using PingCallbackInternal = std::function<void(const milliseconds &time, const std::string &response)>;

public:

    static const std::string ERROR_BAD_REQUEST;

public:
    explicit HttpSimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout);
    //void sendMessageGet(const QUrl &url, const ClientCallback &callback);

    // ping хорошо работает только с максимум одним одновременным запросом
    //void ping(const QString &address, const PingCallback &callback, milliseconds timeout);

    void setParent(QObject *obj);

    void moveToThread(QThread *thread);

Q_SIGNALS:

    void callbackCall(ReturnCallback callback);

Q_SIGNALS:
    void closed();

private slots:
    void onSocketFinished();

    //void onPingReceived();

    void onTimerEvent();

private:

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<class Callbacks, typename... Message>
    void runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages);

    void startTimer();

private:
    //std::unique_ptr<QNetworkAccessManager> manager;
    std::unordered_map<std::string, ClientCallback> callbacks_;
    //std::unordered_map<std::string, PingCallbackInternal> pingCallbacks_;
    //std::unordered_map<std::string, QNetworkReply*> requests;
    std::unordered_map<std::string, HttpSocket *> sockets;

    QTimer* timer = nullptr;

    QThread *thread1 = nullptr;

    int id = 0;
};

#endif // CLIENT_H
