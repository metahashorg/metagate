#include "AppUrlSchemeHandler.h"

/*#include <QWebEngineUrlRequestJob>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>*/

#include "MainWindow.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "check.h"

SET_LOG_NAMESPACE("MW");

const static QNetworkRequest::Attribute REQUEST_ID_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 0);
const static QNetworkRequest::Attribute TIME_BEGIN_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 1);
const static QNetworkRequest::Attribute TIMOUT_FIELD = QNetworkRequest::Attribute(QNetworkRequest::User + 2);

static void addRequestId(QNetworkRequest &request, const std::string &id)
{
    request.setAttribute(REQUEST_ID_FIELD, QString::fromStdString(id));
}

static bool isRequestId(const QNetworkReply &reply) {
    return reply.request().attribute(REQUEST_ID_FIELD).userType() == QMetaType::QString;
}

static std::string getRequestId(const QNetworkReply &reply)
{
    CHECK(isRequestId(reply), "Request id field not set");
    return reply.request().attribute(REQUEST_ID_FIELD).toString().toStdString();
}

static void addBeginTime(QNetworkRequest &request, time_point tp)
{
    request.setAttribute(TIME_BEGIN_FIELD, QString::fromStdString(std::to_string(timePointToInt(tp))));
}

static bool isBeginTime(const QNetworkReply &reply)
{
    return reply.request().attribute(TIME_BEGIN_FIELD).userType() == QMetaType::QString;
}

static time_point getBeginTime(const QNetworkReply &reply)
{
    CHECK(isBeginTime(reply), "begin time field not set");
    const size_t timeBegin = std::stoull(reply.request().attribute(TIME_BEGIN_FIELD).toString().toStdString());
    const time_point timeBeginTp = intToTimePoint(timeBegin);
    return timeBeginTp;
}

static void addTimeout(QNetworkRequest &request, milliseconds timeout)
{
    request.setAttribute(TIMOUT_FIELD, QString::fromStdString(std::to_string(timeout.count())));
}

static bool isTimeout(const QNetworkReply &reply)
{
    return reply.request().attribute(TIMOUT_FIELD).userType() == QMetaType::QString;
}

static milliseconds getTimeout(const QNetworkReply &reply)
{
    CHECK(isTimeout(reply), "Timeout field not set");
    return milliseconds(std::stol(reply.request().attribute(TIMOUT_FIELD).toString().toStdString()));
}

AppUrlSchemeHandler::AppUrlSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    QFile
    Q_CONNECT(&timer, &QTimer::timeout, this, &TorUrlSchemeHandler::onTimerEvent);
    timer.setInterval(milliseconds(1s).count());
    timer.start();
}

void AppUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    const QUrl url = job->requestUrl();
    QUrl newurl(url);
    if (newurl.scheme() == QLatin1String("tors"))
        newurl.setScheme(QStringLiteral("https"));
    else
        newurl.setScheme(QStringLiteral("http"));
    qDebug() << "Tes " << newurl;
    QNetworkRequest req(newurl);
    quint64 reqId = 0;
    reqId = requestId++;
    addRequestId(req, std::to_string(reqId));
    const time_point time = ::now();
    addBeginTime(req, time);
    addTimeout(req, 5s);

    QNetworkReply *reply = m_manager->get(req);
    reply->setParent(job);
    Q_CONNECT(reply, &QNetworkReply::finished, this, &TorUrlSchemeHandler::onRequestFinished);
}


void AppUrlSchemeHandler::onRequestFinished()
{
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    if (isRequestId(*reply)) {
        const auto requestId = getRequestId(*reply);
        removeOnRequestId(requestId);
    }

    QWebEngineUrlRequestJob *job = qobject_cast<QWebEngineUrlRequestJob *>(reply->parent());
    if (!job) {
        return;
    }

    if (reply->error()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    QVariant contentMimeType = reply->header(QNetworkRequest::ContentTypeHeader);
    QByteArray mime = contentMimeType.toByteArray();
    const int pos = mime.indexOf(';');
    if (pos != -1) {
        mime = mime.left(pos);
    }

    job->reply(mime, reply);
END_SLOT_WRAPPER
}

void AppUrlSchemeHandler::onTimerEvent()
{
BEGIN_SLOT_WRAPPER
    const time_point timeEnd = ::now();
    const auto it = std::stable_partition(requests.begin(), requests.end(), [timeEnd](const QNetworkReply *reply) {
        if (isTimeout(*reply)) {
            const milliseconds timeout = getTimeout(*reply);
            const time_point timeBegin = getBeginTime(*reply);
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(timeEnd - timeBegin);
            if (duration >= timeout) {
                LOG << "Timeout request";
                return false;
            }
        }
        return true;
    });
    std::vector<QNetworkReply*> toDelete(it, requests.end());
    requests.erase(it, requests.end());

    for (QNetworkReply *reply: toDelete) {
        reply->abort();
    }
END_SLOT_WRAPPER
}

/*
void TorUrlSchemeHandler::processRequest(QWebEngineUrlRequestJob *job, MainWindow *win, const QUrl &url, const QString &host, const std::set<QString> &excludesIps)
{
    CHECK(win, "mainwin cast");
    const QString ip = win->getServerIp(url.toString(), excludesIps);
    if (ip.isEmpty()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    QUrl newurl(url);
    newurl.setScheme(QStringLiteral("http"));
    newurl.setHost(ip);
    LOG << "TorUrlSchemeHandler: " << url.toString() << " " << ip << " " << host << " " << newurl.toString();
    QNetworkRequest req(newurl);
    size_t reqId = 0;
    if (isFirstRun) {
        reqId = requestId++;
        addRequestId(req, std::to_string(reqId));
        addIgnoreError(req);
        const time_point time = ::now();
        addBeginTime(req, time);
        addTimeout(req, 5s);
    }
    req.setRawHeader(QByteArray("Host"), host.toUtf8());
    QNetworkReply *reply = m_manager->get(req);
    reply->setParent(job);
    Q_CONNECT(reply, &QNetworkReply::finished, this, &MHUrlSchemeHandler::onRequestFinished);
    if (isFirstRun) {
        Q_CONNECT3(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), ([this, job, win, url, host, ip, excludesIps](QNetworkReply::NetworkError ) {
        BEGIN_SLOT_WRAPPER
            LOG << "Error request MHUrlSchemeHandler " << ip;
            std::set<QString> copyExcludes = excludesIps;
            copyExcludes.insert(ip);
            processRequest(job, win, url, host, copyExcludes);
        END_SLOT_WRAPPER
        }));
        requests.emplace_back(reply);
        Q_CONNECT3(job, &QWebEngineUrlRequestJob::destroyed, ([this, reqIdStr=std::to_string(reqId)]() {
        BEGIN_SLOT_WRAPPER
            removeOnRequestId(reqIdStr);
        END_SLOT_WRAPPER
        }));
    }
    isFirstRun = false;
}
*/
void AppUrlSchemeHandler::removeOnRequestId(const std::string &requestId)
{
    requests.erase(std::remove_if(requests.begin(), requests.end(), [&requestId](const QNetworkReply *reply) {
        if (isRequestId(*reply)) {
            const auto reqId = getRequestId(*reply);
            return reqId == requestId;
        }
        return false;
    }), requests.end());
}
