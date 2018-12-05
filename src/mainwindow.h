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
namespace auth {
class AuthJavascript;
class Auth;
}
namespace messenger {
class MessengerJavascript;
}

namespace Ui {
    class MainWindow;
}

namespace initializer {
class InitializerJavascript;
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

class TypedException;

namespace transactions {
class TransactionsJavascript;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    using SignalFunc = std::function<void(const std::function<void()> &callback)>;

    using SetJavascriptWrapperCallback = std::function<void(const TypedException &exception)>;

    using SetAuthJavascriptCallback = std::function<void(const TypedException &exception)>;

    using SetAuthCallback = std::function<void(const TypedException &exception)>;

    using SetMessengerJavascriptCallback = std::function<void(const TypedException &exception)>;

    using SetTransactionsJavascriptCallback = std::function<void(const TypedException &exception)>;

public:

    explicit MainWindow(initializer::InitializerJavascript &initializerJs, QWidget *parent = 0);

    void showExpanded();

    QString getServerIp(const QString &text) const;

    LastHtmlVersion getCurrentHtmls() const;

signals:

    void setJavascriptWrapper(JavascriptWrapper *jsWrapper, const SignalFunc &signal, const SetJavascriptWrapperCallback &callback);

    void setAuthJavascript(auth::AuthJavascript *authJavascript, const SignalFunc &signal, const SetAuthJavascriptCallback &callback);

    void setAuth(auth::Auth *authManager, const SignalFunc &signal, const SetAuthCallback &callback);

    void setMessengerJavascript(messenger::MessengerJavascript *messengerJavascript, const SignalFunc &signal, const SetMessengerJavascriptCallback &callback);

    void setTransactionsJavascript(transactions::TransactionsJavascript *transactionsJavascript, const SignalFunc &signal, const SetTransactionsJavascriptCallback &callback);

private slots:

    void onSetJavascriptWrapper(JavascriptWrapper *jsWrapper, const SignalFunc &signal, const SetJavascriptWrapperCallback &callback);

    void onSetAuthJavascript(auth::AuthJavascript *authJavascript, const SignalFunc &signal, const SetAuthJavascriptCallback &callback);

    void onSetAuth(auth::Auth *authManager, const SignalFunc &signal, const SetAuthCallback &callback);

    void onSetMessengerJavascript(messenger::MessengerJavascript *messengerJavascript, const SignalFunc &signal, const SetMessengerJavascriptCallback &callback);

    void onSetTransactionsJavascript(transactions::TransactionsJavascript *transactionsJavascript, const SignalFunc &signal, const SetTransactionsJavascriptCallback &callback);

private:

    void softReloadPage();

    void softReloadApp();

    void loadUrl(const QString &page);

    void loadFile(const QString &pageName);

    bool currentFileIsEqual(const QString &pageName);

    void configureMenu();

    void doConfigureMenu();

    void registerCommandLine();

    void unregisterCommandLine();

    void enterCommandAndAddToHistory(const QString &text1, bool isAddToHistory, bool isNoEnterDuplicate);

    void addElementToHistoryAndCommandLine(const QString &text, bool isAddToHistory, bool isReplace);

    void qtOpenInBrowser(QString url);

public slots:

    void processEvent(WindowEvent event);

    void updateAppEvent(const QString appVersion, const QString reference, const QString message);

private slots:

    void onCallbackCall(SimpleClient::ReturnCallback callback);

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

    void onLogined(const QString &login);

private:

    MHUrlSchemeHandler *shemeHandler = nullptr;

    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<QWebChannel> channel;

    JavascriptWrapper *jsWrapper = nullptr;

    const LastHtmlVersion lastHtmls;

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
