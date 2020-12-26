#include "ExternalConnector.h"


#include "MainWindow.h"
#include "qt_utilites/QRegister.h"
#include "check.h"

ExternalConnector::ExternalConnector(MainWindow &mainWindow, QObject *parent)
    : CallbackCallWrapper(parent)
    , mainWindow(mainWindow)
{
    Q_CONNECT3(&mainWindow, &MainWindow::urlChanged, [this](const QString &url){
        if (extConnList.checkUrl(url))
            emit urlChanged(url);
    });
    Q_CONNECT3(&mainWindow, &MainWindow::urlEntered, [this](const QString &url){
        if (extConnList.checkUrl(url))
            emit urlEntered(url);
    });

    Q_CONNECT(this, &ExternalConnector::getUrl, this, &ExternalConnector::onGetUrl);
    Q_CONNECT(this, &ExternalConnector::setUrl, this, &ExternalConnector::onSetUrl);

}

void ExternalConnector::onGetUrl(const ExternalConnector::GetUrlCallback &callback)
{
    const QString url = mainWindow.getCurrenttUrl();
    if (extConnList.checkUrl(url))
        callback.emitCallback(url);
    else
        callback.emitCallback(QStringLiteral(""));
}

void ExternalConnector::onSetUrl(const QString &url, const ExternalConnector::SetUrlCallback &callback)
{
    if (extConnList.checkUrl(url))
        mainWindow.setCurrentUrl(url);
    callback.emitCallback();
}
