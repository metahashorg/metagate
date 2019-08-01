#include "SimpleClient.h"

#include <iostream>
#include <memory>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

#include <QNetworkAccessManager>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>

const int SimpleClient::ServerException::BAD_REQUEST_ERROR = QNetworkReply::ProtocolInvalidOperationError;

template<class Callback>
class CallbackWrapImpl {
public:

    using CallbackCall = std::function<void(SimpleClient::ReturnCallback callback)>;

public:

    CallbackWrapImpl(const std::string printedName, const CallbackCall &callbackCall, const Callback &callback, size_t countArgs)
        : printedName(printedName)
        , callbackCall(callbackCall)
        , callback(callback)
        , args(countArgs)
        , filled(countArgs)
    {
        std::fill(filled.begin(), filled.end(), false);
    }

    void process(size_t index, const SimpleClient::Response &response) {
        CHECK(!emitted, "Already emitted " + printedName);
        CHECK(!filled[index], "callback already called " + std::to_string(index) + ". " + printedName);
        this->args[index] = response;
        filled[index] = true;

        const size_t count = calcSettedCount();
        if (count == filled.size()) {
            emit callbackCall(std::bind(callback, this->args));
            emitted = true;
        }
    }

    ~CallbackWrapImpl() {
        if (!emitted) {
            LOG << "Warn. Callback not emitted " << printedName << ". " << calcSettedCount() << "/" << filled.size();
        }
    }

private:

    size_t calcSettedCount() const {
        const size_t count = std::accumulate(filled.begin(), filled.end(), 0, [](size_t old, bool fill) {
            return old + (fill ? 1 : 0);
        });
        return count;
    }

private:

    const std::string printedName;

    const CallbackCall callbackCall;

    const Callback callback;

    std::vector<SimpleClient::Response> args;

    std::vector<bool> filled;

    bool emitted = false;
};

template<class CallbackWrap>
class CallbackWrapPtr {
public:

    CallbackWrapPtr(const std::shared_ptr<CallbackWrap> &callback, size_t index)
        : callback(callback)
        , index(index)
    {}

    template<typename ...Args>
    void operator()(const Args& ...args) const {
        callback->process(index, args...);
    }

private:

    const std::shared_ptr<CallbackWrap> callback;
    const size_t index;
};

bool SimpleClient::ServerException::isTimeout() const {
    return code == QNetworkReply::OperationCanceledError || code == QNetworkReply::TimeoutError;
}

SimpleClient::SimpleClient()
    : manager(new QNetworkAccessManager(this))
{
    Q_REG(SimpleClient::ReturnCallback, "SimpleClient::ReturnCallback");
}

SimpleClient::~SimpleClient() = default;

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
        Q_CONNECT(timer, &QTimer::timeout, this, &SimpleClient::onTimerEvent);
        if (thread1 != nullptr) {
            Q_CONNECT(thread1, &QThread::finished, timer, &QTimer::stop);
        }
        timer->setInterval(milliseconds(1s).count());
        timer->start();
    }
}

void SimpleClient::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    std::vector<std::reference_wrapper<Request>> toDelete;
    const time_point timeEnd = ::now();
    for (auto &iter: requests) {
        Request &request = iter.second;
        auto &reply = *request.reply;

        if (request.isSetTimeout) {
            const milliseconds &timeout = request.timeout;
            const time_point &timeBegin = request.beginTime;
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);
            if (duration >= timeout) {
                LOG << PeriodicLog::make("cl_tm") << "Timeout request";
                toDelete.emplace_back(request);
            }
        }
    }

    for (Request& reply: toDelete) {
        reply.isTimeout = true;
        reply.reply->abort();
    }
END_SLOT_WRAPPER
}

template<typename Callback>
void SimpleClient::sendMessageInternal(
    bool isPost,
    const QUrl &url,
    const QString &message,
    const Callback &callback,
    bool isTimeout,
    milliseconds timeout,
    bool isClearCache,
    bool isQueuedConnection
) {
    const size_t requestId = id++;

    startTimer1();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    const time_point time = ::now();
    Request r;
    r.beginTime = time;
    r.isSetTimeout = isTimeout;
    r.timeout = timeout;
    r.callback = callback;
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
    r.reply = reply;
    Qt::ConnectionType connType = Qt::AutoConnection;
    if (isQueuedConnection) {
        connType = Qt::QueuedConnection;
    }
    Q_CONNECT2(reply, &QNetworkReply::finished, this, std::bind(&SimpleClient::onTextMessageReceived, this, requestId), connType);
    requests[requestId] = r;
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, bool isTimeout, milliseconds timeout, bool isClearCache) {
    sendMessageInternal(true, url, message, callback, isTimeout, timeout, isClearCache, false);
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback) {
    sendMessagePost(url, message, callback, false, milliseconds(0), false);
}

void SimpleClient::sendMessagePost(const QUrl &url, const QString &message, const ClientCallback &callback, milliseconds timeout, bool isClearCache) {
    sendMessagePost(url, message, callback, true, timeout, isClearCache);
}

void SimpleClient::sendMessagesPost(const std::string printedName, const std::vector<QUrl> &urls, const QString &message, const ClientCallbacks &callback, milliseconds timeout) {
    if (urls.empty()) {
        callback({});
        return;
    }
    const auto callbackImpl = std::make_shared<CallbackWrapImpl<ClientCallbacks>>(printedName, std::bind(&SimpleClient::callbackCall, this, _1), callback, urls.size());
    size_t index = 0;
    for (const QUrl &address: urls) {
        const auto callbackNew = CallbackWrapPtr<std::decay_t<decltype(*callbackImpl)>>(callbackImpl, index);
        sendMessagePost(address, message, ClientCallback(callbackNew), timeout);
        index++;
    }
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback, bool isTimeout, milliseconds timeout) {
    sendMessageInternal(false, url, "", callback, isTimeout, timeout, false, false);
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback) {
    sendMessageGet(url, callback, false, milliseconds(0));
}

void SimpleClient::sendMessageGet(const QUrl &url, const ClientCallback &callback, milliseconds timeout) {
    sendMessageGet(url, callback, true, timeout);
}

template<typename... Message>
void SimpleClient::runCallback(size_t id, Message&&... messages) {
    const auto foundCallback = requests.find(id);
    CHECK(foundCallback != requests.end(), "not found callback on id " + std::to_string(id));
    const auto callback = std::bind(foundCallback->second.callback, std::forward<Message>(messages)...);
    emit callbackCall(callback);
    requests.erase(foundCallback);
}

void SimpleClient::onTextMessageReceived(size_t id) {
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    const Request &request = requests.at(id);
    const time_point timeBegin = request.beginTime;
    const time_point timeEnd = ::now();
    const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray content;
        if (reply->isReadable()) {
            content = reply->readAll();
        }
        Response resp;
        resp.response = std::string(content.data(), content.size());
        resp.time = duration;
        runCallback(id, resp);
    } else {
        std::string errorStr;
        if (reply->isReadable()) {
            errorStr = QString(reply->readAll()).toStdString();
        }

        Response resp;
        resp.time = duration;
        if (request.isTimeout) {
            resp.isTimeout = request.isTimeout;
            resp.exception = ServerException(reply->url().toString().toStdString(), QNetworkReply::TimeoutError, "Timeout", errorStr);
        } else {
            resp.exception = ServerException(reply->url().toString().toStdString(), reply->error(), reply->errorString().toStdString(), errorStr);
        }
        runCallback(id, resp);
    }

    reply->deleteLater();
END_SLOT_WRAPPER
}
