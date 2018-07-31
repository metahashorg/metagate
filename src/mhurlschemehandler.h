#ifndef MHURLSCHEMEHANDLER_H
#define MHURLSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>

class QNetworkAccessManager;
class QWebEngineUrlRequestJob;

class MHUrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
public:
    explicit MHUrlSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

    void setLog();

private slots:
    void onRequestFinished();

private:
    QNetworkAccessManager *m_manager;

    bool isLog = false;
};

#endif // MHURLSCHEMEHANDLER_H
