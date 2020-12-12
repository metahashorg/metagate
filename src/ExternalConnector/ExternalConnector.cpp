#include "ExternalConnector.h"


#include "MainWindow.h"
#include "qt_utilites/QRegister.h"
#include "check.h"

ExternalConnector::ExternalConnector(MainWindow &mainWindow, QObject *parent)
    : CallbackCallWrapper(parent)
    , mainWindow(mainWindow)
{
    Q_CONNECT(&mainWindow, &MainWindow::urlChanged, this, &ExternalConnector::urlChanged);

    Q_CONNECT(this, &ExternalConnector::getUrl, this, &ExternalConnector::onGetUrl);
    Q_CONNECT(this, &ExternalConnector::setUrl, this, &ExternalConnector::onSetUrl);

}

void ExternalConnector::onGetUrl(const ExternalConnector::GetUrlCallback &callback)
{
    const QString url = "foooobaaaaar";
    callback.emitCallback(url);
}

void ExternalConnector::onSetUrl(const QString &url, const ExternalConnector::SetUrlCallback &callback)
{
    callback.emitCallback();
}
