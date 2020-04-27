#include "WebView.h"

#include <QDebug>
#include <QWebEngineSettings>

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("MW");

WebPage::WebPage(QObject *parent)
    : QWebEnginePage(parent)
{
    connect(this, &QWebEnginePage::urlChanged, [](const QUrl &url){
        qDebug() << "New URL " << url;
    });

    connect(this, &QWebEnginePage::loadStarted, [](){
        qDebug() << "LOAD started";
    });
}

WebPage::~WebPage()
{
    qDebug() << "Remove page";
}

QWebEnginePage *WebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    if (type != QWebEnginePage::WebBrowserTab)
        return nullptr;
    WebPage *page = new WebPage(this->parent());
    WebView *view = qobject_cast<WebView *>(this->parent());
    if (view)
        view->setPage(page);
    return page;
}


WebView::WebView(QWidget *parent)
    : QWebEngineView(parent)
{
    setPage(new WebPage(this));

    connect(this, &QWebEngineView::renderProcessTerminated,
                [](QWebEnginePage::RenderProcessTerminationStatus termStatus, int statusCode) {
            QString status;
            switch (termStatus) {
            case QWebEnginePage::NormalTerminationStatus:
                status = QLatin1String("Render process normal exit");
                break;
            case QWebEnginePage::AbnormalTerminationStatus:
                status = QLatin1String("Render process abnormal exit");
                break;
            case QWebEnginePage::CrashedTerminationStatus:
                status = QLatin1String("Render process crashed");
                break;
            case QWebEnginePage::KilledTerminationStatus:
                status = QLatin1String("Render process killed");
                break;
            }

            LOG << "Render process exited with code: " << statusCode << " " << status;
        });
    connect(this, &QWebEngineView::urlChanged,
                [](const QUrl &url) {
        qDebug() << "URL " << url;
    });
}

QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type)
{
    return this;
}
