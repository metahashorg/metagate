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
    CHECK(connect(this, &Proxy::proxyStart, this, &Proxy::onProxyStart), "not connect onProxyStart");
    CHECK(connect(this, &Proxy::proxyStop, this, &Proxy::onProxyStop), "not connect onProxyStop");
    CHECK(connect(this, &Proxy::getPort, this, &Proxy::onGetPort), "not connect onGetPort");
    CHECK(connect(this, &Proxy::setPort, this, &Proxy::onSetPort), "not connect onSetPort");

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

}
