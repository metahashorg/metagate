#ifndef MHURLSCHEMEHANDLER_H
#define MHURLSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>

class QNetworkAccessManager;
class QNetworkReply;
class QWebEngineUrlRequestJob;

class MHUrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
public:
    explicit MHUrlSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

private slots:
    void onRequestFinished();

private:
    QNetworkAccessManager *m_manager;
    QWebEngineUrlRequestJob *m_job;
};

#endif // MHURLSCHEMEHANDLER_H
