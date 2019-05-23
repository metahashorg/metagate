#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <map>

#include <QMainWindow>
#include <QWebChannel>

#include "client.h"
#include "uploader.h"

#include "PagesMappings.h"
#include "CallbackWrapper.h"

class QSystemTrayIcon;
class WebSocketClient;
class JavascriptWrapper;
class MHUrlSchemeHandler;
class MHPayUrlSchemeHandler;
namespace auth {
class AuthJavascript;
class Auth;
}
namespace messenger {
class MessengerJavascript;
}
namespace proxy {
class ProxyJavascript;
}

namespace Ui {
class MainWindow;
}

namespace initializer {
class InitializerJavascript;
}

namespace wallet_names {
class WalletNamesJavascript;
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

struct TypedException;

namespace transactions {
class TransactionsJavascript;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    using SetJavascriptWrapperCallback = CallbackWrapper<void()>;

    using SetAuthCallback = CallbackWrapper<void()>;

    using SetMessengerJavascriptCallback = CallbackWrapper<void()>;

    using SetTransactionsJavascriptCallback = CallbackWrapper<void()>;

    using SetProxyJavascriptCallback = CallbackWrapper<void()>;

    using SetWalletNamesJavascriptCallback = CallbackWrapper<void()>;

public:

    explicit MainWindow(initializer::InitializerJavascript &initializerJs, QWidget *parent = 0);

    ~MainWindow();

    void showExpanded();



    QString getServerIp(const QString &text, const std::set<QString> &excludesIps);

    LastHtmlVersion getCurrentHtmls() const;

public slots:
    void showOnTop();

    virtual void setVisible(bool visible) override;

signals:

    void setJavascriptWrapper(JavascriptWrapper *jsWrapper, const SetJavascriptWrapperCallback &callback);

    void setAuth(auth::AuthJavascript *authJavascript, auth::Auth *authManager, const SetAuthCallback &callback);

    void setMessengerJavascript(messenger::MessengerJavascript *messengerJavascript, const SetMessengerJavascriptCallback &callback);

    void setTransactionsJavascript(transactions::TransactionsJavascript *transactionsJavascript, const SetTransactionsJavascriptCallback &callback);

    void setProxyJavascript(proxy::ProxyJavascript *transactionsJavascript, const SetProxyJavascriptCallback &callback);

    void setWalletNamesJavascript(wallet_names::WalletNamesJavascript *walletNamesJavascript, const SetWalletNamesJavascriptCallback &callback);

    void initFinished();

    void processExternalUrl(const QUrl &url);

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;

    virtual void closeEvent(QCloseEvent *event) override;

    virtual void changeEvent(QEvent *event) override;

private slots:

    void onSetJavascriptWrapper(JavascriptWrapper *jsWrapper, const SetJavascriptWrapperCallback &callback);

    void onSetAuth(auth::AuthJavascript *authJavascript, auth::Auth *authManager, const SetAuthCallback &callback);

    void onSetMessengerJavascript(messenger::MessengerJavascript *messengerJavascript, const SetMessengerJavascriptCallback &callback);

    void onSetTransactionsJavascript(transactions::TransactionsJavascript *transactionsJavascript, const SetTransactionsJavascriptCallback &callback);

    void onSetProxyJavascript(proxy::ProxyJavascript *proxyJavascript, const SetProxyJavascriptCallback &callback);

    void onSetWalletNamesJavascript(wallet_names::WalletNamesJavascript *walletNamesJavascript, const SetWalletNamesJavascriptCallback &callback);

    void onInitFinished();

    void onProcessExternalUrl(const QUrl &url);

private:

    void loadPagesMappings();

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

    void setUserName(QString userName);

    void registerWebChannel(const QString &name, QObject *obj);

    void unregisterAllWebChannels();

    void registerAllWebChannels();

public slots:

    void updateHtmlsEvent();

    void updateAppEvent(const QString appVersion, const QString reference, const QString message);

private slots:

    void onCallbackCall(SimpleClient::ReturnCallback callback);

    void onShowContextMenu(const QPoint &point);

    void onJsRun(QString jsString);

    void onSetHasNativeToolbarVariable();

    void onSetCommandLineText(QString text);

    void onSetMappings(QString mapping);

    void onEnterCommandAndAddToHistory(const QString &text);

    void onEnterCommandAndAddToHistoryNoDuplicate(const QString &text);

    void onUrlChanged(const QUrl &url2);

    void onLogined(bool isInit, const QString &login);

private:
    std::unique_ptr<Ui::MainWindow> ui;

    QSystemTrayIcon *systemTray;
    QMenu *trayMenu;
    QAction *hideAction;
    QAction *showAction;

    MHUrlSchemeHandler *shemeHandler = nullptr;

    MHPayUrlSchemeHandler *shemeHandler2 = nullptr;

    std::unique_ptr<QWebChannel> channel;

    JavascriptWrapper *jsWrapper = nullptr;

    LastHtmlVersion last_htmls;
    mutable std::mutex mutLastHtmls;

    QString currentTextCommandLine;

    PagesMappings pagesMappings;

    QString hardwareId;

    size_t countFocusLineEditChanged = 0;

    std::vector<QString> history;
    size_t historyPos = 0;

    SimpleClient client;

    QString prevUrl;
    bool prevIsApp = true;

    QString prevTextCommandLine;

    QString currentUserName;

    QString urlDns;

    QString netDns;

    bool lineEditUserChanged = false;

    bool isInitFinished = false;

    QUrl saveUrlToMove;

    std::vector<std::pair<QString, QObject*>> registeredWebChannels;
    bool isRegisteredWebChannels = true;
};

#endif // MAINWINDOW_H
