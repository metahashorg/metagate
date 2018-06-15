#include "mhurlschemehandler.h"

#include <QWebEngineUrlRequestJob>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "mainwindow.h"
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
    //qDebug() << host;

    QString ip;
    MainWindow *win = qobject_cast<MainWindow *>(parent());
    CHECK(win, "mainwin cast");
    ip = win->getServerIp(url.toString());

    QUrl newurl(url);
    newurl.setScheme(QStringLiteral("http"));
    newurl.setHost(ip);
    //qDebug() << newurl;
    QNetworkRequest req(newurl);
    req.setRawHeader(QByteArray("Host"), host.toUtf8());
    m_job = job;
    QNetworkReply *reply = m_manager->get(req);
    reply->setProperty("job", QVariant::fromValue(job));
    CHECK(connect(reply, &QNetworkReply::finished, this, &MHUrlSchemeHandler::onRequestFinished), "");
}

void MHUrlSchemeHandler::onRequestFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    QWebEngineUrlRequestJob *job = reply->property("job").value<QWebEngineUrlRequestJob *>();
    if (reply->error()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        //reply->deleteLater();
        return;
    }

    QVariant contentMimeType = reply->header(QNetworkRequest::ContentTypeHeader);
    qDebug() << contentMimeType;

    for (auto &i : reply->rawHeaderPairs()) {
        QString str;
        qDebug() << str.sprintf(
                        "%40s: %s",
                        i.first.data(),
                        i.second.data());
    }
    job->reply(contentMimeType.toByteArray(), reply);
    //reply->deleteLater();
}
