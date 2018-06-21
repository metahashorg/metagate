#include "mhurlschemehandler.h"

#include <QWebEngineUrlRequestJob>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "mainwindow.h"
#include "SlotWrapper.h"
#include "check.h"

Q_DECLARE_METATYPE(QWebEngineUrlRequestJob *)

MHUrlSchemeHandler::MHUrlSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent)
{
    m_manager = new QNetworkAccessManager(this);
}

void MHUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    const QUrl url = job->requestUrl();
    const QString host = url.host();

    QString ip;
    MainWindow *win = qobject_cast<MainWindow *>(parent());
    CHECK(win, "mainwin cast");
    ip = win->getServerIp(url.toString());

    QUrl newurl(url);
    newurl.setScheme(QStringLiteral("http"));
    newurl.setHost(ip);
    QNetworkRequest req(newurl);
    req.setRawHeader(QByteArray("Host"), host.toUtf8());
    QNetworkReply *reply = m_manager->get(req);
    reply->setParent(job);
    CHECK(connect(reply, &QNetworkReply::finished, this, &MHUrlSchemeHandler::onRequestFinished), "connect fail");
}

void MHUrlSchemeHandler::onRequestFinished()
{
BEGIN_SLOT_WRAPPER
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }
    QWebEngineUrlRequestJob *job = qobject_cast<QWebEngineUrlRequestJob *>(reply->parent());
    if (!job) {
        return;
    }
    if (reply->error()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    QVariant contentMimeType = reply->header(QNetworkRequest::ContentTypeHeader);
    QByteArray mime = contentMimeType.toByteArray();
    const int pos = mime.indexOf(';');
    if (pos != -1) {
        mime = mime.left(pos);
    }

    job->reply(mime, reply);
END_SLOT_WRAPPER
}
