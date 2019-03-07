#include "mhpayurlschemehandler.h"

#include <QWebEngineUrlRequestJob>

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("MW");

MHPayUrlSchemeHandler::MHPayUrlSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent)
{
}

void MHPayUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job) {
    const QUrl url = job->requestUrl();
    const QString host = url.host();
    QUrl newUrl("http://google.com");
    newUrl.setPath(url.path());
    newUrl.setQuery(url.query());
    newUrl.setFragment(url.fragment());

    job->redirect(newUrl);
}
