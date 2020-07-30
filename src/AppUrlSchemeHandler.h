#ifndef APPURLSCHEMEHANDLER_H
#define APPURLSCHEMEHANDLER_H

#include <set>
#include <unordered_map>
#include <atomic>

#include <QTimer>
#include <QWebEngineUrlSchemeHandler>

class QWebEngineUrlRequestJob;
class MainWindow;
class QNetworkReply;

class AppUrlSchemeHandler : public QWebEngineUrlSchemeHandler {
public:
    explicit AppUrlSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

private slots:
    void onRequestFinished();

    void onTimerEvent();

private:

    //void processRequest(QWebEngineUrlRequestJob *job, MainWindow *win, const QUrl &url, const QString &host, const std::set<QString> &excludesIps);

    void removeOnRequestId(const std::string &requestId);

private:

    std::vector<QNetworkReply*> requests;

    QTimer timer;

    std::atomic<quint64> requestId{0};
};

#endif // APPURLSCHEMEHANDLER_H
