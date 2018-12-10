#include "ProxyJavascript.h"

#include "Proxy.h"

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"

#include <QDebug>

namespace proxy
{

ProxyJavascript::ProxyJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &ProxyJavascript::sendServerStatusResponseSig, this, &ProxyJavascript::onSendServerStatusResponseSig), "not connect onSendServerStatusResponseSig");
    CHECK(connect(this, &ProxyJavascript::sendServerPortResponseSig, this, &ProxyJavascript::onSendServerPortResponseSig), "not connect onSendServerPortResponseSig");
}

void ProxyJavascript::proxyStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxy not set");

    qDebug() << "!!!";
    LOG << "Proxy start";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->proxyStart();
    });
    //if (exception.isSet()) {
    //    emit sendLoginInfoResponseSig(LoginInfo(), exception);
    //}
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyStop()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxy not set");
    LOG << "Proxy stop";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->proxyStop();
    });
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

qDebug() << "SEND!";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, port);
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
