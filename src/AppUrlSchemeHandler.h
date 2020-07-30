#ifndef MHURLSCHEMEHANDLER_H
#define MHURLSCHEMEHANDLER_H

#include <set>
#include <unordered_map>
#include <atomic>

#include <QTimer>
#include <QWebEngineUrlSchemeHandler>

class QNetworkAccessManager;
class QWebEngineUrlRequestJob;
class MainWindow;
class QNetworkReply;

class MHUrlSchemeHandler : public QWebEngineUrlSchemeHandler {
public:
    explicit MHUrlSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

    void setLog();

    void setFirstRun();

private slots:
    void onRequestFinished();

    void onTimerEvent();

private:

    void processRequest(QWebEngineUrlRequestJob *job, MainWindow *win, const QUrl &url, const QString &host, const std::set<QString> &excludesIps);

    void removeOnRequestId(const std::string &requestId);

private:
    QNetworkAccessManager *m_manager;

    bool isLog = false;

    bool isFirstRun = false;

    std::vector<QNetworkReply*> requests;

    QTimer timer;

    std::atomic<size_t> requestId{0};

};

#endif // MHURLSCHEMEHANDLER_H
