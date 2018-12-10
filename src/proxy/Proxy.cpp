#include "Proxy.h"

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
{
    qRegisterMetaType<std::vector<Proxy::Router>>("std::vector<Proxy::Router>");

    qRegisterMetaType<PortMappingCallback>("PortMappingCallback");

    CHECK(connect(this, &Proxy::proxyStart, this, &Proxy::onProxyStart), "not connect onProxyStart");
    CHECK(connect(this, &Proxy::proxyStop, this, &Proxy::onProxyStop), "not connect onProxyStop");
    CHECK(connect(this, &Proxy::getPort, this, &Proxy::onGetPort), "not connect onGetPort");
    CHECK(connect(this, &Proxy::setPort, this, &Proxy::onSetPort), "not connect onSetPort");
    CHECK(connect(this, &Proxy::getRouters, this, &Proxy::onGetRouters), "not connect onGetRouters");
    CHECK(connect(this, &Proxy::addPortMapping, this, &Proxy::onAddPortMapping), "not connect onAddPortMapping");
    CHECK(connect(this, &Proxy::deletePortMapping, this, &Proxy::onDeletePortMapping), "not connect onDeletePortMapping");

    CHECK(connect(upnp, &UPnPDevices::discovered, this, &Proxy::onRouterDiscovered), "not connect onRouterDiscovered");

    upnp->discover();
    javascriptWrapper.setProxyManager(*this);

    thread.start();
    moveToThread(&thread);
}

Proxy::~Proxy()
{
    thread.quit();
    if (!thread.wait(3000)) {
        thread.terminate();
        thread.wait();
    }
}

void Proxy::onProxyStart()
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->start();
    });
END_SLOT_WRAPPER
    //emit javascriptWrapper.sendLoginInfoResponseSig(info, exception);
}

void Proxy::onProxyStop()
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->stop();
    });
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

void Proxy::onAddPortMapping(const Proxy::PortMappingCallback &callback)
{
BEGIN_SLOT_WRAPPER
    bool result;
    const TypedException exception = apiVrapper2([&, this] {
        quint16 port = proxyServer->port();
        routers.at(0).router->addPortMapping(port+1, port, TCP, [this, callback](bool result, const QString &error){
            qDebug() << "!!!!!!!!!!!!!!!!!!!!11";
            runCallback(std::bind(callback, result, TypedException()));
        });
    });

        //runCallback(std::bind(callback, false, exception));
END_SLOT_WRAPPER
}

void Proxy::onDeletePortMapping(const Proxy::PortMappingCallback &callback)
{
BEGIN_SLOT_WRAPPER
    bool result;
    const TypedException exception = apiVrapper2([&, this] {
        quint16 port = proxyServer->port();
        routers.at(0).router->deletePortMapping(port, TCP, [this, callback](bool result, const QString &error){
            qDebug() << "!!!!!!!!!!!!!!!!!!!!11";
            runCallback(std::bind(callback, result, TypedException()));
        });
    });

    //runCallback(std::bind(callback, false, exception));
END_SLOT_WRAPPER
}

void Proxy::onRouterDiscovered(UPnPRouter *router)
{
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
    routers.push_back(r);
}

template<typename Func>
void Proxy::runCallback(const Func &callback) {
    emit javascriptWrapper.callbackCall(callback);
}

}
