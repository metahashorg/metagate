#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QUrl>

#include <memory>
#include <functional>
#include <map>
#include <string>

#include "duration.h"

struct TypedException;

class AbstractSocket: public QTcpSocket {
    Q_OBJECT
public:

    AbstractSocket(QObject *parent)
        : QTcpSocket(parent)
    {}

    virtual ~AbstractSocket() = default;

signals:
    void finished();

public:

    void stop();

    int requestId() const;
    void setRequestId(const int s);

    bool hasTimeOut() const;
    bool hasError() const;

    time_point timePoint() const;
    void setTimePoint(time_point s);

    milliseconds timeOut() const;
    void setTimeOut(milliseconds s);

    QByteArray getReply() const;

    int errorC() const {
        return errorCode;
    }

private:
    int m_requestId;
    time_point m_timePoint;
    milliseconds m_timeOut;
    bool m_hasTimeOut = false;

protected:

    QByteArray m_reply;

    bool m_error = false;
    int errorCode = 0;
};

class HttpSocket : public AbstractSocket
{
    Q_OBJECT
public:
    explicit HttpSocket(const QUrl &url, const QString &message, QObject *parent = nullptr);

    void start();

private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();

private:
    QByteArray getHttpPostHeader() const;
    void parseResponseHeader();

    QUrl m_url;
    QString m_message;
    QByteArray m_data;
    bool m_headerParsed = false;
    int m_contentLength = -1;
    bool m_firstHeaderStringParsed = false;
};

class PingSocket : public AbstractSocket
{
    Q_OBJECT
public:
    explicit PingSocket(const QUrl &url, QObject *parent = nullptr);

    void start();

private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();

private:

    QUrl m_url;
    QByteArray m_data;
};

/*
   На каждый поток должен быть один экземпляр класса.
   */
class HttpSimpleClient : public QObject
{
    Q_OBJECT

public:
    using ClientCallback = std::function<void(const std::string &response, const TypedException &exception)>;
    using ReturnCallback = std::function<void()>;

public:
    explicit HttpSimpleClient();

    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback);
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout);

    void sendMessagePing(const QUrl &url, const ClientCallback &callback, milliseconds timeout);

    void moveToThread(QThread *thread);

Q_SIGNALS:

    void callbackCall(HttpSimpleClient::ReturnCallback callback);

Q_SIGNALS:
    void closed();

private slots:
    void onSocketFinished();
    void onTimerEvent();

private:
    void sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout);

    template<class Callbacks, typename... Message>
    void runCallback(Callbacks &callbacks, const int id, Message&&... messages);

    void startTimer1();

private:
    std::map<int, ClientCallback> callbacks;
    std::map<int, AbstractSocket *> sockets;

    QTimer* timer = nullptr;
    QThread *thread1 = nullptr;

    int id = 0;
};

#endif // HTTP_CLIENT_H
