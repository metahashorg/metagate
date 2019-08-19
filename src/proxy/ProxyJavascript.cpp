#include "ProxyJavascript.h"

#include "Proxy.h"

#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/makeJsFunc.h"
#include "qt_utilites/QRegister.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include "qt_utilites/WrapperJavascriptImpl.h"

SET_LOG_NAMESPACE("PRX");

namespace proxy
{

static QJsonDocument routersInfoToJson(const std::vector<Proxy::Router> &routers)
{
    QJsonArray routersJson;
    for (const Proxy::Router &router: routers) {
        QJsonObject rJson;
        rJson.insert("friendlyName", router.friendlyName);
        rJson.insert("manufacturer", router.manufacturer);
        rJson.insert("modelDescription", router.modelDescription);
        rJson.insert("modelName", router.modelName);
        rJson.insert("modelNumber", router.modelNumber);
        rJson.insert("serialNumber", router.serialNumber);
        rJson.insert("udn", router.udn);
        rJson.insert("isMapped", router.mapped);
        routersJson.push_back(rJson);
    }

    return QJsonDocument(routersJson);
}

static QJsonDocument proxyStatusToJson(const Proxy::ProxyStatus &status)
{
    QJsonObject resJson;
    resJson.insert("started", status.started);
    resJson.insert("autoActive", status.autoActive);
    resJson.insert("portMapped", status.portMapped);
    return QJsonDocument(resJson);
}

static QJsonDocument proxyResultToJson(const Proxy::ProxyResult &res)
{
    QJsonObject resJson;
    resJson.insert("ok", res.ok);
    resJson.insert("error", res.error);
    return QJsonDocument(resJson);
}

static QJsonDocument portMappingResultToJson(const Proxy::PortMappingResult &res)
{
    QJsonObject resJson;
    resJson.insert("ok", res.ok);
    resJson.insert("port", res.port);
    resJson.insert("udn", res.udn);
    resJson.insert("error", res.error);
    return QJsonDocument(resJson);
}

ProxyJavascript::ProxyJavascript(QObject *parent)
    : WrapperJavascript(false, LOG_FILE)
{
    Q_CONNECT(this, &ProxyJavascript::sendServerStatusResponseSig, this, &ProxyJavascript::onSendServerStatusResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendServerPortResponseSig, this, &ProxyJavascript::onSendServerPortResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendGetRoutersResponseSig, this, &ProxyJavascript::onSendGetRoutersResponseSig);

    Q_CONNECT(this, &ProxyJavascript::sendAutoStartExecutedResponseSig, this, &ProxyJavascript::onSendAutoStartExecutedResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendAutoStartProxyResponseSig, this, &ProxyJavascript::onSendAutoStartProxyResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendAutoStartRouterResponseSig, this, &ProxyJavascript::onSendAutoStartRouterResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendAutoStartTestResponseSig, this, &ProxyJavascript::onSendAutoStartTestResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendAutoStartCompleteResponseSig, this, &ProxyJavascript::onSendAutoStartCompleteResponseSig);
    Q_CONNECT(this, &ProxyJavascript::sendAutoStartIsActiveResponseSig, this, &ProxyJavascript::onSendAutoStartIsActiveResponseSig);

    Q_CONNECT(this, &ProxyJavascript::sendConnectedPeersResponseSig, this, &ProxyJavascript::onSendConnectedPeersResponseSig);
}

void ProxyJavascript::proxyStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    LOG << "Proxy start";

    const QString JS_NAME_RESULT = "proxyStartResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit m_proxyManager->proxyStart([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
            LOG << "Proxt started " << res.ok << res.error;
            makeFunc.func(exception, proxyResultToJson(res));
        });
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyStop()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy stop";

    const QString JS_NAME_RESULT = "proxyStopResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit m_proxyManager->proxyStop([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
            LOG << "Proxt stoped " << res.ok << res.error;
            makeFunc.func(exception, proxyResultToJson(res));
        });
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyJavascript::getProxyStatus()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Get proxy status";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->onGeProxyStatus();
    });

    if (exception.isSet()) {
        emit sendServerStatusResponseSig(Proxy::ProxyStatus(false, false, false), exception);
    }
END_SLOT_WRAPPER
}

void ProxyJavascript::getPort()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Get port";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->getPort();
    });
    if (exception.isSet()) {
        emit sendServerPortResponseSig(0, exception);
    }
END_SLOT_WRAPPER
}

void ProxyJavascript::setPort(quint16 port)
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Set port " << port;

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->setPort(port);
    });
    if (exception.isSet()) {
        emit sendServerPortResponseSig(0, exception);
    }
END_SLOT_WRAPPER
}

void ProxyJavascript::getRoutersList()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Get routers list";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->getRouters();
    });
    if (exception.isSet()) {
        //emit sendServerPortResponseSig(std::vector<Proxy::Router>(), exception);
    }
END_SLOT_WRAPPER
}

void ProxyJavascript::discoverRouters()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Start routers discover";

    const QString JS_NAME_RESULT = "proxyDiscoverRoutersResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<bool>(false));

    wrapOperation([&, this]() {
        emit m_proxyManager->discoverRouters([makeFunc](bool res, const TypedException &exception) {
            LOG << "Started routers discover " << res;
            makeFunc.func(exception, res);
        });
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyJavascript::addPortMapping(const QString &udn)
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    const QString JS_NAME_RESULT = "proxyAddPortMappingResultJs";

    LOG << "Add Port Mapping to router " << udn;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit m_proxyManager->addPortMapping(udn, [makeFunc](const Proxy::PortMappingResult &res, const TypedException &exception) {
            LOG << "Added port mapping " << res.ok << res.error;
            makeFunc.func(exception, portMappingResultToJson(res));
        });
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyJavascript::deletePortMapping()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    const QString JS_NAME_RESULT = "proxyDeletePortMappingResultJs";

    LOG << "Delete Port Mapping ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit m_proxyManager->deletePortMapping([makeFunc](const Proxy::PortMappingResult &res, const TypedException &exception) {
            LOG << "Deleted port mapping " << res.ok << res.error;
            makeFunc.func(exception, portMappingResultToJson(res));
        });
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyAutoStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy auto start";

    const QString JS_NAME_RESULT = "proxyAutoStartResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    emit m_proxyManager->autoStart([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
        LOG << "Proxy auto exec started " << res.ok << res.error;
        makeFunc.func(exception, proxyResultToJson(res));
    });
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyAutoStop()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy auto stop";

    const QString JS_NAME_RESULT = "proxyAutoStopResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    emit m_proxyManager->autoStop([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
        LOG << "Proxy stoped " << res.ok << res.error;
        makeFunc.func(exception, proxyResultToJson(res));
    });
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyAutoStartResend()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy autostart resend";
    emit m_proxyManager->autoStartResend();
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendServerStatusResponseSig(const Proxy::ProxyStatus &status, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyServerStatusInfoJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, proxyStatusToJson(status));
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendServerPortResponseSig(quint16 port, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyServerPortInfoJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, port);
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendGetRoutersResponseSig(const std::vector<Proxy::Router> &routers, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyGetRoutersJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, routersInfoToJson(routers));
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendAutoStartExecutedResponseSig(const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyAutoStartExecutedResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error);
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendAutoStartProxyResponseSig(const Proxy::ProxyResult &res, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyAutoStartProxyResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, proxyResultToJson(res));
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendAutoStartRouterResponseSig(const Proxy::ProxyResult &res, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyAutoStartRouterResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, proxyResultToJson(res));
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendAutoStartTestResponseSig(const Proxy::ProxyResult &res, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyAutoStartTestResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, proxyResultToJson(res));
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendAutoStartCompleteResponseSig(const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyAutoStartCompleteResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error);
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendAutoStartIsActiveResponseSig(bool active, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyAutoStartIsActiveResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, active);
END_SLOT_WRAPPER
}

void ProxyJavascript::onSendConnectedPeersResponseSig(int num, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyConnectedPeersResJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, num);
END_SLOT_WRAPPER
}

}
