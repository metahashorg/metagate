#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <map>

#include <QMainWindow>
#include <QWebChannel>
#include <QTimer>

#include "client.h"
#include "uploader.h"

#include "ui_mainwindow.h"

#include "PagesMappings.h"

#include "WindowEvents.h"

class WebSocketClient;
class JavascriptWrapper;
class MHUrlSchemeHandler;
class MessengerJavascript;

namespace Ui {
    class MainWindow;
}

class EvFilter: public QObject {
    Q_OBJECT
public:

    EvFilter(QObject *parent, const QString &icoActive, const QString &icoHover)
        : QObject(parent)
        , icoActive(icoActive)
        , icoHover(icoHover)
    {}

    bool eventFilter(QObject * watched, QEvent * event);

    QIcon icoActive;
    QIcon icoHover;
};

namespace transactions {
class TransactionsJavascript;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    explicit MainWindow(WebSocketClient &webSocketClient, JavascriptWrapper &jsWrapper, MessengerJavascript &messengerJavascript, transactions::TransactionsJavascript &transactionsJavascript, const QString &applicationVersion, QWidget *parent = 0);

    void showExpanded();

    QString getServerIp(const QString &text) const;

private:

    void softReloadPage();

    void softReloadApp();

    void loadUrl(const QString &page);

    void loadFile(const QString &pageName);

    void configureMenu();

    void registerCommandLine();

    void unregisterCommandLine();

    void sendAppInfoToWss(bool force);

    void enterCommandAndAddToHistory(const QString &text1, bool isAddToHistory, bool isNoEnterDuplicate);

    void addElementToHistoryAndCommandLine(const QString &text, bool isAddToHistory, bool isReplace);

    void qtOpenInBrowser(QString url);

public slots:

    void processEvent(WindowEvent event);

    void updateAppEvent(const QString appVersion, const QString reference, const QString message);

private slots:

    void onCallbackCall(ReturnCallback callback);

    void onUpdateMhsReferences();

    void onShowContextMenu(const QPoint &point);

    void onJsRun(QString jsString);

    void onSetHasNativeToolbarVariable();

    void onSetCommandLineText(QString text);

    void onSetUserName(QString userName);

    void onSetMappings(QString mapping);

    void onEnterCommandAndAddToHistory(const QString &text);

    void onEnterCommandAndAddToHistoryNoDuplicate(const QString &text);

    void onBrowserLoadFinished(const QUrl &url2);

private:

    MHUrlSchemeHandler *shemeHandler = nullptr;

    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<QWebChannel> channel;

    WebSocketClient &webSocketClient;

    JavascriptWrapper &jsWrapper;

    QString sendedUserName;

    const QString applicationVersion;

    LastHtmlVersion lastHtmls;

    QString currentTextCommandLine;

    PagesMappings pagesMappings;

    QString hardwareId;

    size_t countFocusLineEditChanged = 0;

    std::vector<QString> history;
    size_t historyPos = 0;

    SimpleClient client;

    QTimer qtimer;

    QString prevUrl;

    QString prevTextCommandLine;

    bool lineEditUserChanged = false;
};

#endif // MAINWINDOW_H
