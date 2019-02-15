#include "Proxy.h"

#include <QTimer>
#include <QSettings>

#include "ProxyJavascript.h"
#include "ProxyServer.h"
#include "UPnPDevices.h"
#include "UPnPRouter.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Paths.h"

namespace proxy
{

Proxy::Proxy(ProxyJavascript &javascriptWrapper, QObject *parent)
    : QObject(parent)
    , thread(parent)
    , m_isAutoStart(false)
    , javascriptWrapper(javascriptWrapper)
    , state(No)
    , proxyServer(new ProxyServer(this))
    , upnp(new UPnPDevices(this))
    , mappedRouterIdx(-1)
    , autoActive(false)
    , m_peers(0)
    , proxyStarted(false)
    , portMapped(false)
{
    qRegisterMetaType<std::vector<Proxy::Router>>("std::vector<Proxy::Router>");
    qRegisterMetaType<Proxy::ProxyResult>("Proxy::ProxyResult");

    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<ProxyCallback>("ProxyCallback");
    qRegisterMetaType<DiscoverCallback>("DiscoverCallback");
    qRegisterMetaType<PortMappingCallback>("PortMappingCallback");

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("mgproxy/port"), "mgproxy/port not found setting");
    proxyServer->setPort(settings.value("mgproxy/port").toUInt());

    CHECK(connect(this, &Proxy::proxyStart, this, &Proxy::onProxyStart), "not connect onProxyStart");
    CHECK(connect(this, &Proxy::proxyStop, this, &Proxy::onProxyStop), "not connect onProxyStop");
    CHECK(connect(this, &Proxy::geProxyStatus, this, &Proxy::onGeProxyStatus), "not connect onGeProxyStatus");
    CHECK(connect(this, &Proxy::getPort, this, &Proxy::onGetPort), "not connect onGetPort");
    CHECK(connect(this, &Proxy::setPort, this, &Proxy::onSetPort), "not connect onSetPort");
    CHECK(connect(this, &Proxy::getRouters, this, &Proxy::onGetRouters), "not connect onGetRouters");
    CHECK(connect(this, &Proxy::discoverRouters, this, &Proxy::onDiscoverRouters), "not connect onDiscoverRouters");
    CHECK(connect(this, &Proxy::addPortMapping, this, &Proxy::onAddPortMapping), "not connect onAddPortMapping");
    CHECK(connect(this, &Proxy::deletePortMapping, this, &Proxy::onDeletePortMapping), "not connect onDeletePortMapping");
    CHECK(connect(this, &Proxy::autoStart, this, &Proxy::onAutoStart), "not connect onAutoStart");
    CHECK(connect(this, &Proxy::autoStop, this, &Proxy::onAutoStop), "not connect onAutoStop");
    CHECK(connect(this, &Proxy::autoStartResend, this, &Proxy::onAutoStartResend), "not connect onAutoStartResend");

    CHECK(connect(upnp, &UPnPDevices::discovered, this, &Proxy::onRouterDiscovered), "not connect onRouterDiscovered");

    CHECK(connect(proxyServer, &ProxyServer::connectedPeersChanged, this, &Proxy::onConnedtedPeersChanged), "not connect onConnedtedPeersChanged");

    javascriptWrapper.setProxyManager(*this);
    thread.start();
    moveToThread(&thread);

    CHECK(settings.contains("mgproxy/autostart"), "mgproxy/autostart not found setting");
    m_isAutoStart = settings.value("mgproxy/autostart").toBool();
    if (m_isAutoStart)
        QMetaObject::invokeMethod(this, "startAutoProxy");
    upnp->discover();
}

Proxy::~Proxy()
{
    QMetaObject::invokeMethod(this, "delPortMapping", Qt::QueuedConnection, Q_ARG(Callback, [this](){
        thread.quit();
    }));
    thread.quit();
    if (!thread.wait(3000)) {
        thread.terminate();
        thread.wait();
    }
}

bool Proxy::isAutoStart() const
{
    return m_isAutoStart;
}

void Proxy::startAutoProxy()
{
    if (state != No && state != AutoError)
        return;
    // Start proxy
    LOG << "Proxy auto start: executed";
    autoActive = true;
    emit startAutoExecued();
    emit javascriptWrapper.sendAutoStartExecutedResponseSig(TypedException());
    state = AutoExecuted;
    //proxyServer->setPort(22);
    bool res = proxyServer->start();
    if (!res) {
        LOG << "Proxy auto start: proxy start error";
        emit startAutoProxyResult(TypedException(PROXY_PROXY_START_ERROR, "Proxy start error"));
        emit javascriptWrapper.sendAutoStartProxyResponseSig(ProxyResult(false, "Proxy start error"), TypedException());
        autoProxyRes = false;
        state = AutoProxyStarted;
        autoActive = false;
        return;
    }
    LOG << "Proxy auto start: proxy start ok";
    emit startAutoProxyResult(TypedException(NOT_ERROR, "Proxy started"));
    emit javascriptWrapper.sendAutoStartProxyResponseSig(ProxyResult(true, "Proxy started"), TypedException());
    autoProxyRes = true;
    state = AutoProxyStarted;
    // Wait 10 sec to find routers
    QTimer::singleShot(10 * 1000, this, &Proxy::onAutoDiscoveryTimeout);
}

void Proxy::proxyTested(bool res, const QString &error)
{
    //qDebug() << res;
    // LOG ??
    emit javascriptWrapper.sendAutoStartTestResponseSig(ProxyResult(res, error), TypedException());
    autoTestRes = res;
    if (res)
        emit javascriptWrapper.sendAutoStartCompleteResponseSig(TypedException());
    emit startAutoComplete1(res);
    autoActive = false;
    proxyStarted = res;
    state = AutoComplete;
    emit javascriptWrapper.sendConnectedPeersResponseSig(m_peers, TypedException());
}

void Proxy::onProxyStart(const ProxyCallback &callback)
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->start();
        proxyStarted = true;
        ProxyResult res(true, QStringLiteral("OK"));
        runCallback(std::bind(callback, res, TypedException()));
    });

    if (exception.isSet()) {
        ProxyResult res(false, QStringLiteral("Exception"));
        runCallback(std::bind(callback, res, exception));
    }
END_SLOT_WRAPPER
}

void Proxy::onProxyStop(const ProxyCallback &callback)
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->stop();
        proxyStarted = false;
        ProxyResult res(true, QStringLiteral("OK"));
        runCallback(std::bind(callback, res, TypedException()));
    });

    if (exception.isSet()) {
        ProxyResult res(false, QStringLiteral("Exception"));
        runCallback(std::bind(callback, res, exception));
    }
END_SLOT_WRAPPER
}

void Proxy::onGeProxyStatus()
{
BEGIN_SLOT_WRAPPER
    ProxyStatus status(proxyStarted, autoActive, portMapped);
    emit javascriptWrapper.sendServerStatusResponseSig(status, TypedException());
END_SLOT_WRAPPER
}

void Proxy::onGetPort()
{
BEGIN_SLOT_WRAPPER
    emit javascriptWrapper.sendServerPortResponseSig(proxyServer->port(), TypedException());
END_SLOT_WRAPPER
}

void Proxy::onSetPort(quint16 port)
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->setPort(port);
    });
    emit javascriptWrapper.sendServerPortResponseSig(proxyServer->port(), exception);
END_SLOT_WRAPPER
}

void Proxy::onGetRouters()
{
BEGIN_SLOT_WRAPPER
    emit javascriptWrapper.sendGetRoutersResponseSig(routers, TypedException());
END_SLOT_WRAPPER
}

void Proxy::onDiscoverRouters(const DiscoverCallback &callback)
{
BEGIN_SLOT_WRAPPER
    if (mappedRouterIdx != -1) {
        qDebug() << "Discover disabled. Port mapped.";
        runCallback(std::bind(callback, false, TypedException()));
        return;
    }
    routers.clear();
    upnp->discover();
    runCallback(std::bind(callback, true, TypedException()));
END_SLOT_WRAPPER
}

void Proxy::onAddPortMapping(const QString &udn, const Proxy::PortMappingCallback &callback)
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        quint16 port = proxyServer->port();
        int ridx = findRouter(udn);
        if (ridx == -1) {
            PortMappingResult res(false, 0, QStringLiteral(""), QStringLiteral("Router not found"));
            runCallback(std::bind(callback, res, TypedException()));
            return ;
        }
        routers.at(ridx).router->addPortMapping(port, port, TCP, [this, callback, port, ridx](bool r, const QString &error) {
            //qDebug() << "Added port " << r;
            if (r)
                mappedRouterIdx = ridx;
            routers[ridx].mapped = true;
            PortMappingResult res(r, port, routers[mappedRouterIdx].udn, error);
            runCallback(std::bind(callback, res, TypedException()));
        });
    });

    if (exception.isSet()) {
        PortMappingResult res(false, 0, QStringLiteral(""), QStringLiteral("Exception"));
        runCallback(std::bind(callback, res, exception));
    }
END_SLOT_WRAPPER
}

void Proxy::onDeletePortMapping(const Proxy::PortMappingCallback &callback)
{
BEGIN_SLOT_WRAPPER
    if (mappedRouterIdx == -1) {
        PortMappingResult res(false, 0, QStringLiteral(""), QStringLiteral("No port is mapped"));
        runCallback(std::bind(callback, res, TypedException()));
        return ;
    }
    CHECK(mappedRouterIdx < routers.size(), "mappedRouterIdx error");
    const TypedException exception = apiVrapper2([&, this] {
        quint16 port = proxyServer->port();
        routers.at(0).router->deletePortMapping(port, TCP, [this, callback, port](bool r, const QString &error) {
            qDebug() << "Deleted port";
            QString udn = routers[mappedRouterIdx].udn;
            routers[mappedRouterIdx].mapped = false;
            if (r)
                mappedRouterIdx = -1;
            PortMappingResult res(r, port, udn, error);
            runCallback(std::bind(callback, res, TypedException()));
        });
    });

    if (exception.isSet()) {
        PortMappingResult res(false, 0, QStringLiteral(""), QStringLiteral("Exception"));
        runCallback(std::bind(callback, res, exception));
    }
END_SLOT_WRAPPER
}

void Proxy::onAutoStart(const ProxyCallback &callback)
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        startAutoProxy();
        ProxyResult res(true, QStringLiteral("OK"));
        runCallback(std::bind(callback, res, TypedException()));
    });

    if (exception.isSet()) {
        ProxyResult res(false, QStringLiteral("Exception"));
        runCallback(std::bind(callback, res, exception));
    }
END_SLOT_WRAPPER
}

void Proxy::onAutoStop(const ProxyCallback &callback)
{
BEGIN_SLOT_WRAPPER
    if (state != AutoComplete) {
        ProxyResult res(false, QStringLiteral("State is not Complete"));
        runCallback(std::bind(callback, res, TypedException()));
        return;
    }
    const TypedException exception = apiVrapper2([&, this] {
        qDebug() << "!!!!!!! stop";
        proxyStarted = false;
        state = No;
        proxyServer->stop();
        //delPortMapping();
        delPortMapping([](){});

    });
    if (exception.isSet()) {
        ProxyResult res(false, QStringLiteral("Exception"));
        runCallback(std::bind(callback, res, exception));
    }
END_SLOT_WRAPPER
}

void Proxy::onAutoStartResend()
{
    emit  javascriptWrapper.sendAutoStartIsActiveResponseSig(autoActive, TypedException());
    switch (state) {
    case AutoExecuted:
        emit javascriptWrapper.sendAutoStartExecutedResponseSig(TypedException());
        break;
    case AutoProxyStarted:
        emit javascriptWrapper.sendAutoStartExecutedResponseSig(TypedException());
        emit javascriptWrapper.sendAutoStartProxyResponseSig(ProxyResult(autoProxyRes, ""), TypedException());
        break;
    case AutoUPNPDone:
        emit javascriptWrapper.sendAutoStartExecutedResponseSig(TypedException());
        emit javascriptWrapper.sendAutoStartProxyResponseSig(ProxyResult(autoProxyRes, ""), TypedException());
        emit javascriptWrapper.sendAutoStartRouterResponseSig(ProxyResult(autoRouterRes, ""), TypedException());
        break;
    case AutoComplete:
        emit javascriptWrapper.sendAutoStartExecutedResponseSig(TypedException());
        emit javascriptWrapper.sendAutoStartProxyResponseSig(ProxyResult(autoProxyRes, ""), TypedException());
        emit javascriptWrapper.sendAutoStartRouterResponseSig(ProxyResult(autoRouterRes, ""), TypedException());
        emit javascriptWrapper.sendAutoStartTestResponseSig(ProxyResult(autoTestRes, ""), TypedException());
        if (autoTestRes)
            emit javascriptWrapper.sendAutoStartCompleteResponseSig(TypedException());
        break;
    }
    emit javascriptWrapper.sendConnectedPeersResponseSig(m_peers, TypedException());
}

void Proxy::onRouterDiscovered(UPnPRouter *router)
{
    LOG << "Router discovered " << router->friendlyName();
    Router r;
    r.router = router;
    r.friendlyName = router->friendlyName();
    r.manufacturer = router->manufacturer();
    r.modelDescription = router->modelDescription();
    r.modelName = router->modelName();
    r.modelNumber = router->modelNumber();
    r.serialNumber = router->serialNumber();
    r.udn = router->udn();
    r.mapped = false;
    routers.push_back(r);

    emit javascriptWrapper.sendGetRoutersResponseSig(routers, TypedException());
}

void Proxy::onAutoDiscoveryTimeout()
{
    if (routers.empty()) {
        LOG << "Proxy auto start: no routers found";
        emit startAutoUPnPResult(TypedException(PROXY_UPNP_ROUTER_NOT_FOUND, "Routers not found"));
        emit javascriptWrapper.sendAutoStartRouterResponseSig(ProxyResult(false, "Routers not found"), TypedException());
        autoRouterRes = false;
        state = AutoUPNPDone;
        LOG << "Proxy auto start: complete";
        emit startAutoReadyToTest(proxyServer->port());
        return;
    }

    // Add port mapping to 1st router
    routers.at(0).router->addPortMapping(proxyServer->port(), proxyServer->port(), proxy::TCP, [this](bool r, const QString &error) {
        qDebug() << "Added port " << r;
        portMapped = r;
        if (r) {
            LOG << "Proxy auto start: add port mapping ok";
            emit startAutoUPnPResult(TypedException(NOT_ERROR, "Port mapping ok"));
            emit javascriptWrapper.sendAutoStartRouterResponseSig(ProxyResult(true, "Port mapping ok"), TypedException());
            autoRouterRes = true;
        } else {
            LOG << "Proxy auto start: add port mapping error";
            emit startAutoUPnPResult(TypedException(PROXY_UPNP_ADD_PORT_MAPPING_ERROR, error.toStdString()));
            emit javascriptWrapper.sendAutoStartRouterResponseSig(ProxyResult(false, error), TypedException());
            autoRouterRes = false;
        }
        state = AutoUPNPDone;
        LOG << "Proxy auto start: complete";
        emit startAutoReadyToTest(proxyServer->port());
    });
}

void Proxy::onConnedtedPeersChanged(int peers)
{
    m_peers = peers;
    emit javascriptWrapper.sendConnectedPeersResponseSig(m_peers, TypedException());
}

int Proxy::findRouter(const QString &udn) const
{
    for (int i = 0; i < routers.size(); i++) {
        if (routers[i].udn == udn)
            return i;
    }
    return -1;
}

void Proxy::delPortMapping(const Callback &callback)
{
    if (!portMapped)
        return;
    quint16 port = proxyServer->port();
    if (routers.empty())
        return;
    //routers.at(0).router->deletePortMapping(port, TCP, [this, port](bool r, const QString &error) {
    routers.at(0).router->deletePortMapping(port, TCP, [this, port, callback](bool r, const QString &error) {
        qDebug() << "Deleted port " << r;
        callback();
    });
    portMapped = false;
}

template<typename Func>
void Proxy::runCallback(const Func &callback) {
    emit javascriptWrapper.callbackCall(callback);
}

}
