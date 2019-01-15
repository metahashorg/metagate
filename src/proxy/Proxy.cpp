#include "Proxy.h"

#include <QTimer>

#include "ProxyJavascript.h"
#include "ProxyServer.h"
#include "UPnPDevices.h"
#include "UPnPRouter.h"

#include "check.h"
#include "SlotWrapper.h"

namespace proxy
{

Proxy::Proxy(ProxyJavascript &javascriptWrapper, QObject *parent)
    : QObject(parent)
    , javascriptWrapper(javascriptWrapper)
    , proxyServer(new ProxyServer(this))
    , upnp(new UPnPDevices(this))
    , mappedRouterIdx(-1)
{
    qRegisterMetaType<std::vector<Proxy::Router>>("std::vector<Proxy::Router>");

    qRegisterMetaType<ProxyCallback>("ProxyCallback");
    qRegisterMetaType<DiscoverCallback>("DiscoverCallback");
    qRegisterMetaType<PortMappingCallback>("PortMappingCallback");

    CHECK(connect(this, &Proxy::proxyStart, this, &Proxy::onProxyStart), "not connect onProxyStart");
    CHECK(connect(this, &Proxy::proxyStop, this, &Proxy::onProxyStop), "not connect onProxyStop");
    CHECK(connect(this, &Proxy::geProxyStatus, this, &Proxy::onGeProxyStatus), "not connect onGeProxyStatus");
    CHECK(connect(this, &Proxy::getPort, this, &Proxy::onGetPort), "not connect onGetPort");
    CHECK(connect(this, &Proxy::setPort, this, &Proxy::onSetPort), "not connect onSetPort");
    CHECK(connect(this, &Proxy::getRouters, this, &Proxy::onGetRouters), "not connect onGetRouters");
    CHECK(connect(this, &Proxy::discoverRouters, this, &Proxy::onDiscoverRouters), "not connect onDiscoverRouters");
    CHECK(connect(this, &Proxy::addPortMapping, this, &Proxy::onAddPortMapping), "not connect onAddPortMapping");
    CHECK(connect(this, &Proxy::deletePortMapping, this, &Proxy::onDeletePortMapping), "not connect onDeletePortMapping");

    CHECK(connect(upnp, &UPnPDevices::discovered, this, &Proxy::onRouterDiscovered), "not connect onRouterDiscovered");

    javascriptWrapper.setProxyManager(*this);
    thread.start();
    moveToThread(&thread);

    upnp->discover();
}

Proxy::~Proxy()
{
    thread.quit();
    if (!thread.wait(3000)) {
        thread.terminate();
        thread.wait();
    }
}

void Proxy::startAutoProxy()
{
    // Start proxy
    qDebug() << "Start";
    //proxyServer->setPort(22);
    bool res = proxyServer->start();
    if (!res) {
        emit startAutoProxyResult(TypedException(PROXY_PROXY_START_ERROR, "Proxy start error"));
        return;
    }
    // Wait 10 sec to find routers
    QTimer::singleShot(10 * 1000, this, &Proxy::onAutoDiscoveryTimeout);
}

void Proxy::proxyTested(bool res)
{
    qDebug() << res;
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
    ProxyStatus status(proxyStarted);
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

void Proxy::onRouterDiscovered(UPnPRouter *router)
{
    qDebug() << "!!!!!";
    LOG << "Router discovered" << router->friendlyName();
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
        qDebug() << "NO routers";
        emit startAutoProxyResult(TypedException(PROXY_UPNP_ROUTER_NOT_FOUND, "Routers not found"));
        return;
    }

    // Add port mapping to 1st router
    routers.at(0).router->addPortMapping(proxyServer->port(), proxyServer->port(), proxy::TCP, [this](bool r, const QString &error) {
        qDebug() << "Added port " << r;
        if (!r) {
            emit startAutoProxyResult(TypedException(PROXY_UPNP_ADD_PORT_MAPPING_ERROR, error.toStdString()));
            return;
        }
        emit startAutoProxyResult(TypedException(PROXY_OK, "Proxy ok"));
    });
}

int Proxy::findRouter(const QString &udn) const
{
    for (int i = 0; i < routers.size(); i++) {
        if (routers[i].udn == udn)
            return i;
    }
    return -1;
}

template<typename Func>
void Proxy::runCallback(const Func &callback) {
    emit javascriptWrapper.callbackCall(callback);
}

}
