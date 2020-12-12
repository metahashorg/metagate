#include "ExternalConnectorManager.h"

#include "ExternalConnector.h"

#include "qt_utilites/QRegister.h"
#include "qt_utilites/SlotWrapper.h"

#include "Network/LocalClient.h"
#include "Network/LocalServer.h"

#include "Paths.h"
#include "check.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace
{
const QString UrlChangedMethod = QLatin1String("UrlChanged");
const QString GetUrlMethod = QLatin1String("GetUrl");
const QString SetUrlMethod = QLatin1String("SetUrl");

struct GetUrlResponse
{
    bool error = false;
    QString url;
};

QByteArray makeUrlChangedRequest(const QString& url)
{
    QJsonObject json;
    json.insert(QStringLiteral("method"), UrlChangedMethod);
    json.insert(QStringLiteral("url"), url);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

bool parseUrlChangedResponse(const QByteArray& message)
{
    bool error = true;
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(message);
    CHECK(jsonResponse.isObject(), "Incorrect json");
    const QJsonObject& root = jsonResponse.object();

    CHECK(
        root.contains(QLatin1String("result"))
            && root.value(QLatin1String("result")).isString(),
        "result field not found");
    const QString rt = root.value(QLatin1String("result")).toString();
    if (rt == QLatin1String("OK"))
        error = false;
    else
        error = true;
    return error;
}

QByteArray makeGetUrlResponse(const QString& url)
{
    QJsonObject json;
    json.insert(QStringLiteral("result"), QStringLiteral("OK"));
    json.insert(QStringLiteral("url"), url);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QByteArray makeSetUrlResponse()
{
    QJsonObject json;
    json.insert(QStringLiteral("result"), QStringLiteral("OK"));
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString parseMethodAtRequest(const QJsonDocument& request)
{
    CHECK(request.isObject(), "Request field not found");
    const QJsonObject root = request.object();
    CHECK(
        root.contains("method") && root.value("method").isString(),
        "'method' field not found");
    const QString method = root.value("method").toString();
    return method;
}

QString parseSetUrlRequest(const QJsonDocument& request)
{
    CHECK(request.isObject(), "Request field not found");
    const QJsonObject root = request.object();
    CHECK(root.contains("url") && root.value("url").isString(), "'url' field not found");
    return root.value("url").toString();
}

} // namespace

ExternalConnectorManager::ExternalConnectorManager(ExternalConnector &externalConnector, QObject* parent)
    : TimerClass(5min, parent)
    , externalConnector(externalConnector)
    , localServer(new localconnection::LocalServer(getExternalConnectionInLocalServerPath(), this))
    , localClient(new localconnection::LocalClient(getExternalConnectionOutLocalServerPath(), this))
{
    Q_CONNECT(
        &externalConnector,
        &ExternalConnector::urlChanged,
        this,
        &ExternalConnectorManager::urlChanged);

    Q_CONNECT(
        localServer,
        &localconnection::LocalServer::request,
        this,
        &ExternalConnectorManager::onRequest);
}

ExternalConnectorManager::~ExternalConnectorManager()
{
    TimerClass::exit();
}

void ExternalConnectorManager::startMethod()
{
}

void ExternalConnectorManager::timerMethod()
{
}

void ExternalConnectorManager::finishMethod()
{
}

void ExternalConnectorManager::urlChanged(const QString& url)
{
    BEGIN_SLOT_WRAPPER
    localClient->sendRequest(
        makeUrlChangedRequest(url),
        [](const localconnection::LocalClient::Response& response) {
            BEGIN_SLOT_WRAPPER
            QString status;
            const TypedException exception = apiVrapper2([&] {
                CHECK_TYPED(
                    !response.exception.isSet(),
                    TypeErrors::EXTERCTORCONNECTOR_LOCALCONN_ERROR,
                    response.exception.toString());
                const bool result = parseUrlChangedResponse(response.response);
                CHECK_TYPED(
                    !result,
                    TypeErrors::EXTERCTORCONNECTOR_URLCHANGED_ERROR,
                    "UrlChanged error");
            });
            END_SLOT_WRAPPER
        });
    END_SLOT_WRAPPER
}

void ExternalConnectorManager::onRequest(
    std::shared_ptr<localconnection::LocalServerRequest> request)
{
    qDebug() << request->request();
    const QJsonDocument reqJson = QJsonDocument::fromJson(request->request());
    const QString method = parseMethodAtRequest(reqJson);
    if (method == GetUrlMethod)
    {
        const auto cb = ExternalConnector::GetUrlCallback(
            [&request](const QString &url) {
                qDebug() << url << QThread::currentThread();
                request->response(makeGetUrlResponse(url));
            },
            [](const TypedException &){},
            signalFunc);
        emit externalConnector.getUrl(cb);
    }
    else if (method == SetUrlMethod)
    {
        const QString url = parseSetUrlRequest(reqJson);
        const auto cb = ExternalConnector::SetUrlCallback(
            [&request]() {
                qDebug() << QThread::currentThread();
                request->response(makeSetUrlResponse());
            },
            [](const TypedException &){},
            signalFunc);
        emit externalConnector.setUrl(url, cb);
    }
}
