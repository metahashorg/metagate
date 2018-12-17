#include "ProxyJavascript.h"

#include "Proxy.h"

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"

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

    CHECK(connect(this, &ProxyJavascript::callbackCall, this, &ProxyJavascript::onCallbackCall), "not connect onCallbackCall");
}

void ProxyJavascript::proxyStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxyManager not set");

    qDebug() << "!!!";
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

void ProxyJavascript::onSendServerStatusResponseSig(bool connected, const TypedException &error)
{
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "proxyServerStatusInfoJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, error, connected);
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
