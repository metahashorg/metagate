#include "ProxyJavascript.h"

#include "Proxy.h"

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"
#include "QRegister.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

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
    : QObject(parent)
{
    CHECK(connect(this, &ProxyJavascript::sendServerStatusResponseSig, this, &ProxyJavascript::onSendServerStatusResponseSig), "not connect onSendServerStatusResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendServerPortResponseSig, this, &ProxyJavascript::onSendServerPortResponseSig), "not connect onSendServerPortResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendGetRoutersResponseSig, this, &ProxyJavascript::onSendGetRoutersResponseSig), "not connect onSendGetRoutersResponseSig");

    CHECK(connect(this, &ProxyJavascript::sendAutoStartExecutedResponseSig, this, &ProxyJavascript::onSendAutoStartExecutedResponseSig), "not connect onSendAutoStartExecutedResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendAutoStartProxyResponseSig, this, &ProxyJavascript::onSendAutoStartProxyResponseSig), "not connect onSendAutoStartProxyResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendAutoStartRouterResponseSig, this, &ProxyJavascript::onSendAutoStartRouterResponseSig), "not connect onSendAutoStartRouterResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendAutoStartTestResponseSig, this, &ProxyJavascript::onSendAutoStartTestResponseSig), "not connect onSendAutoStartTestResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendAutoStartCompleteResponseSig, this, &ProxyJavascript::onSendAutoStartCompleteResponseSig), "not connect onSendAutoStartCompleteResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendAutoStartIsActiveResponseSig, this, &ProxyJavascript::onSendAutoStartIsActiveResponseSig), "not connect onSendAutoStartIsActiveResponseSig");

    CHECK(connect(this, &ProxyJavascript::sendConnectedPeersResponseSig, this, &ProxyJavascript::onSendConnectedPeersResponseSig), "not connect onSendConnectedPeersResponseSig");

    CHECK(connect(this, &ProxyJavascript::callbackCall, this, &ProxyJavascript::onCallbackCall), "not connect onCallbackCall");

    Q_REG(ProxyJavascript::Callback, "ProxyJavascript::Callback");
}

void ProxyJavascript::proxyStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    LOG << "Proxy start";

    const QString JS_NAME_RESULT = "proxyStartResultJs";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->proxyStart([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
            LOG << "Proxt started " << res.ok << res.error;
            makeFunc(exception, proxyResultToJson(res));
        });
    });

    if (exception.isSet()) {
        //makeFunc(exception, false);
    }

END_SLOT_WRAPPER
}

void ProxyJavascript::proxyStop()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy stop";

    const QString JS_NAME_RESULT = "proxyStopResultJs";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->proxyStop([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
            LOG << "Proxt stoped " << res.ok << res.error;
            makeFunc(exception, proxyResultToJson(res));
        });
    });

    if (exception.isSet()) {
        //makeFunc(exception, false);
    }
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, bool result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->discoverRouters([makeFunc](bool res, const TypedException &exception) {
            LOG << "Started routers discover " << res;
            makeFunc(exception, res);
        });
    });
    if (exception.isSet()) {
        makeFunc(exception, false);
    }
END_SLOT_WRAPPER
}

void ProxyJavascript::addPortMapping(const QString &udn)
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    const QString JS_NAME_RESULT = "proxyAddPortMappingResultJs";

    LOG << "Add Port Mapping to router " << udn;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->addPortMapping(udn, [makeFunc](const Proxy::PortMappingResult &res, const TypedException &exception) {
            LOG << "Added port mapping " << res.ok << res.error;
            makeFunc(exception, portMappingResultToJson(res));
        });
    });

    if (exception.isSet()) {
        //makeFunc(exception, false);
    }
END_SLOT_WRAPPER
}

void ProxyJavascript::deletePortMapping()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    const QString JS_NAME_RESULT = "proxyDeletePortMappingResultJs";

    LOG << "Delete Port Mapping ";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->deletePortMapping([makeFunc](const Proxy::PortMappingResult &res, const TypedException &exception) {
            LOG << "Deleted port mapping " << res.ok << res.error;
            makeFunc(exception, portMappingResultToJson(res));
        });
    });

    if (exception.isSet()) {
        //makeFunc(exception, false);
    }
    END_SLOT_WRAPPER
}

void ProxyJavascript::proxyAutoStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy auto start";

    const QString JS_NAME_RESULT = "proxyAutoStartResultJs";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    emit m_proxyManager->autoStart([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
        LOG << "Proxy auto exec started " << res.ok << res.error;
        makeFunc(exception, proxyResultToJson(res));
    });
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyAutoStop()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");
    LOG << "Proxy auto stop";

    const QString JS_NAME_RESULT = "proxyAutoStopResultJs";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    emit m_proxyManager->autoStop([makeFunc](const Proxy::ProxyResult &res, const TypedException &exception) {
        LOG << "Proxy stoped " << res.ok << res.error;
        makeFunc(exception, proxyResultToJson(res));
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

void ProxyJavascript::onCallbackCall(const ProxyJavascript::Callback &callback)
{
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

template<typename... Args>
void ProxyJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void ProxyJavascript::runJs(const QString &script)
{
    LOG << "Js " << script;
    emit jsRunSig(script);
}

}
