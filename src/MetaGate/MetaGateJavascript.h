#ifndef METAGATEJAVASCRIPT_H
#define METAGATEJAVASCRIPT_H

#include "qt_utilites/WrapperJavascript.h"

namespace metagate {

class MetaGate;

class MetaGateJavascript: public WrapperJavascript{
    Q_OBJECT
public:

    MetaGateJavascript(MetaGate &metagate);

public:

    Q_INVOKABLE void updateAndReloadApplication(const QString &callback);

    Q_INVOKABLE void exitApplication();

    Q_INVOKABLE void restartBrowser();

    Q_INVOKABLE void getAppInfoCallback(const QString &callback);

    Q_INVOKABLE void lineEditReturnPressed(const QString &text);

    Q_INVOKABLE void metaOnline(const QString &callback);

    Q_INVOKABLE void clearNsLookup(const QString &callback);

    Q_INVOKABLE void sendMessageToWss(const QString &message, const QString &callback);

    Q_INVOKABLE void setForgingActive(bool isActive, const QString &callback);

    Q_INVOKABLE void getForgingIsActive(const QString &callback);

    Q_INVOKABLE void getNetworkStatus(const QString &callback);

private slots:

    void onMetaOnlineResponse(const QString &response);

    void onShowExchangePopup(const QString &type);

private:

    MetaGate &metagate;
};

} // namespace metagate

#endif // METAGATEJAVASCRIPT_H
