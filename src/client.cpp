#include "client.h"

#include <iostream>
#include <memory>
using namespace std::placeholders;

#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"
#include "QRegister.h"

#include <QNetworkAccessManager>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QThread>

QT_USE_NAMESPACE

const static QNetworkRequest::Attribute REQUEST_ID_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 0);
const static QNetworkRequest::Attribute TIME_BEGIN_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 1);
const static QNetworkRequest::Attribute TIMOUT_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 2);

const int SimpleClient::ServerException::BAD_REQUEST_ERROR = QNetworkReply::ProtocolInvalidOperationError;

template<class Callback, typename ...Args>
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

    void process(size_t index, const Args& ...args) {
        CHECK(!emitted, "Already emitted " + printedName);
        CHECK(!filled[index], "callback already called " + std::to_string(index) + ". " + printedName);
        this->args[index] = std::tuple<Args...>(args...);
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

    std::vector<std::tuple<Args...>> args;

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

static void addRequestId(QNetworkRequest &request, const std::string &id) {
    request.setAttribute(REQUEST_ID_FIELD, QString::fromStdString(id));
}

static bool isRequestId(const QNetworkReply &reply) {
    return reply.request().attribute(REQUEST_ID_FIELD).userType() == QMetaType::QString;
}

static std::string getRequestId(const QNetworkReply &reply) {
    CHECK(isRequestId(reply), "Request id field not set"); // Эта ошибка обычно обозначает, что с url что-то не то
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
                LOG << PeriodicLog::make("cl_tm") << "Timeout request";
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
    Q_CONNECT2(reply, &QNetworkReply::finished, this, onTextMessageReceived, connType);
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

void SimpleClient::sendMessagesPost(const std::string printedName, const std::vector<QUrl> &urls, const QString &message, const ClientCallbacks &callback, milliseconds timeout) {
    if (urls.empty()) {
        callback({});
        return;
    }
    const auto callbackImpl = std::make_shared<CallbackWrapImpl<ClientCallbacks, std::string, ServerException>>(printedName, std::bind(&SimpleClient::callbackCall, this, _1), callback, urls.size());
    size_t index = 0;
    for (const QUrl &address: urls) {
        const auto callbackNew = CallbackWrapPtr<std::decay_t<decltype(*callbackImpl)>>(callbackImpl, index);
        sendMessagePost(address, message, ClientCallback(callbackNew), timeout);
        index++;
    }
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

void SimpleClient::pings(const std::string printedName, const std::vector<QString> &addresses, const PingsCallback &callback, milliseconds timeout) {
    if (addresses.empty()) {
        callback({});
        return;
    }
    const auto callbackImpl = std::make_shared<CallbackWrapImpl<PingsCallback, QString, milliseconds, std::string>>(printedName, std::bind(&SimpleClient::callbackCall, this, _1), callback, addresses.size());
    size_t index = 0;
    for (const QString &address: addresses) {
        const auto callbackNew = CallbackWrapPtr<std::decay_t<decltype(*callbackImpl)>>(callbackImpl, index);
        ping(address, PingCallback(callbackNew), timeout);
        index++;
    }
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

        runCallback(callbacks_, requestId, "", ServerException(reply->url().toString().toStdString(), reply->error(), reply->errorString().toStdString(), errorStr));
    }

    reply->deleteLater();
END_SLOT_WRAPPER
}
