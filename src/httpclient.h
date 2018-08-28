#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

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
using ReturnCallback = std::function<void()>;

class HttpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit HttpSocket(const QUrl &url, const QString &message, QObject *parent = nullptr);

    void start();

    std::string requestId() const;
    void setRequestId(const std::string &s);

    bool hasTimeOut() const;
    bool hasError() const;

    time_point timePoint() const;
    void setTimePoint(time_point s);

    milliseconds timeOut() const;
    void setTimeOut(milliseconds s);

    QByteArray getReply() const;

signals:
    void finished();

private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError socketError);
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
    bool m_error = false;
    bool m_firstHeaderStringParsed = false;
};

/*
   На каждый поток должен быть один экземпляр класса.
   */
class HttpSimpleClient : public QObject
{
    Q_OBJECT

public:

    static const std::string ERROR_BAD_REQUEST;

public:
    explicit HttpSimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout);

    void setParent(QObject *obj);

    void moveToThread(QThread *thread);

Q_SIGNALS:

    void callbackCall(ReturnCallback callback);

Q_SIGNALS:
    void closed();

private slots:
    void onSocketFinished();
    void onTimerEvent();

private:
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<class Callbacks, typename... Message>
    void runCallback(Callbacks &callbacks, const std::string &id, Message&&... messages);

    void startTimer();

private:
    std::unordered_map<std::string, ClientCallback> callbacks_;
    std::unordered_map<std::string, HttpSocket *> sockets;

    QTimer* timer = nullptr;

    QThread *thread1 = nullptr;

    int id = 0;
};

#endif // HTTP_CLIENT_H
