#ifndef TORURLSCHEMEHANDLER_H
#define TORURLSCHEMEHANDLER_H

#include <set>
#include <unordered_map>
#include <atomic>

#include <QTimer>
#include <QNetworkProxy>
#include <QWebEngineUrlSchemeHandler>

class QNetworkAccessManager;
class QWebEngineUrlRequestJob;
class MainWindow;
class QNetworkReply;

class TorUrlSchemeHandler : public QWebEngineUrlSchemeHandler {
public:
    explicit TorUrlSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

private slots:
    void onRequestFinished();

    void onTimerEvent();

private:

    //void processRequest(QWebEngineUrlRequestJob *job, MainWindow *win, const QUrl &url, const QString &host, const std::set<QString> &excludesIps);

    void removeOnRequestId(const std::string &requestId);

private:
    QNetworkAccessManager *m_manager;
    QNetworkProxy proxy;

//    bool isFirstRun = false;

    std::vector<QNetworkReply*> requests;

    QTimer timer;

    std::atomic<quint64> requestId{0};
};

#endif // TORURLSCHEMEHANDLER_H
