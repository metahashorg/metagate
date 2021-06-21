#include "MainWindow.h"

#include <map>
#include <mutex>

#include <QWebEnginePage>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QTextDocument>
#include <QLineEdit>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QKeyEvent>
#include <QApplicationStateChangeEvent>
#include <QMenu>
#include <QStandardItemModel>
#include <QFontDatabase>
#include <QDesktopServices>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QDesktopWidget>

#include "ui_MainWindow.h"

#include "Network/WebSocketClient.h"
#include "JavascriptWrapper.h"

#include "Uploader.h"
#include "check.h"
#include "StopApplication.h"
#include "duration.h"
#include "Log.h"
#include "utilites/utils.h"
#include "qt_utilites/SlotWrapper.h"
#include "Paths.h"
#include "qt_utilites/QRegister.h"

#include "TorUrlSchemeHandler.h"
#include "TorProxy.h"
#include "MHUrlSchemeHandler.h"

#include "auth/AuthJavascript.h"
#include "auth/Auth.h"
#include "Messenger/MessengerJavascript.h"
#include "transactions/TransactionsJavascript.h"
#include "Initializer/InitializerJavascript.h"
#include "WalletNames/WalletNamesJavascript.h"
#include "Utils/UtilsJavascript.h"
#include "Wallets/WalletsJavascript.h"
#include "MetaGate/MetaGateJavascript.h"
#include "MetaGate/MetaGate.h"
#include "ProxyClient/ProxyClientJavascript.h"

#include "utilites/machine_uid.h"

SET_LOG_NAMESPACE("MW");

const static QString DEFAULT_USERNAME = "_unregistered";

bool EvFilter::eventFilter(QObject * watched, QEvent * event) {
    QToolButton * button = qobject_cast<QToolButton*>(watched);
    if (!button) {
        return false;
    }

    if (event->type() == QEvent::Enter) {
        // The push button is hovered by mouse
        button->setIcon(icoHover);
        return true;
    } else if (event->type() == QEvent::Leave){
        // The push button is not hovered by mouse
        button->setIcon(icoActive);
        return true;
    }

    return false;
}


void WebUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    qDebug() << info.requestUrl();
    QUrl url = info.requestUrl();
    if (url.host().endsWith(QLatin1String(".onion"))) {
        if (url.scheme() == QLatin1String("http"))
        {
            url.setScheme(QStringLiteral("tor"));
            qDebug() << url;
            info.redirect(url);
        } else if (url.scheme() == QLatin1String("https"))
        {
            url.setScheme(QStringLiteral("tors"));
            qDebug() << url;
            info.redirect(url);
        }
    }
}

MainWindow::MainWindow(initializer::InitializerJavascript &initializerJs, tor::TorProxy &torProxy, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , systemTray(new QSystemTrayIcon(QIcon(":/resources/svg/systemtray.png"), this))
    , trayMenu(new QMenu(this))
    , last_htmls(Uploader::getLastHtmlVersion())
    , currentUserName(DEFAULT_USERNAME)
{
    ui->setupUi(this);
    systemTray->setVisible(true);
    systemTray->setContextMenu(trayMenu);

    hideAction = new QAction(tr("&Hide"), this);
    connect(hideAction, &QAction::triggered, this, &QWidget::hide);

    showAction = new QAction(tr("&Show"), this);
    connect(showAction, &QAction::triggered, this, &MainWindow::showOnTop);

    QAction *quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    trayMenu->addAction(hideAction);
    trayMenu->addAction(showAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

#ifndef Q_OS_MACOS
    connect(systemTray, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        BEGIN_SLOT_WRAPPER
        if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
            return;

        if (this->isVisible()) {
            this->setVisible(false);
            /*if (this->isActiveWindow())
                this->setVisible(false);
            else
                showOnTop();*/
        } else {
            showOnTop();
        }
        END_SLOT_WRAPPER
    });
#endif
    qApp->installEventFilter(this);

    Q_CONNECT(this, &MainWindow::setJavascriptWrapper, this, &MainWindow::onSetJavascriptWrapper);
    Q_CONNECT(this, &MainWindow::setAuth, this, &MainWindow::onSetAuth);
    Q_CONNECT(this, &MainWindow::setMessengerJavascript, this, &MainWindow::onSetMessengerJavascript);
    Q_CONNECT(this, &MainWindow::setTransactionsJavascript, this, &MainWindow::onSetTransactionsJavascript);
    Q_CONNECT(this, &MainWindow::setWalletNamesJavascript, this, &MainWindow::onSetWalletNamesJavascript);
    Q_CONNECT(this, &MainWindow::setUtilsJavascript, this, &MainWindow::onSetUtilsJavascript);
    Q_CONNECT(this, &MainWindow::setWalletsJavascript, this, &MainWindow::onSetWalletsJavascript);
    Q_CONNECT(this, &MainWindow::setMetaGateJavascript, this, &MainWindow::onSetMetaGateJavascript);
    Q_CONNECT(this, &MainWindow::setProxyJavascript, this, &MainWindow::onSetProxyJavascript);
    Q_CONNECT(this, &MainWindow::initFinished, this, &MainWindow::onInitFinished);
    Q_CONNECT(this, &MainWindow::processExternalUrl, this, &MainWindow::onProcessExternalUrl);
    Q_CONNECT(this, &MainWindow::showNotification, this, &MainWindow::onShowNotification);

    Q_REG(SetJavascriptWrapperCallback, "SetJavascriptWrapperCallback");
    Q_REG(SetAuthCallback, "SetAuthCallback");
    Q_REG(SetMessengerJavascriptCallback, "SetMessengerJavascriptCallback");
    Q_REG(SetTransactionsJavascriptCallback, "SetTransactionsJavascriptCallback");
    Q_REG(SetWalletNamesJavascriptCallback, "SetWalletNamesJavascriptCallback");
    Q_REG(SetUtilsJavascriptCallback, "SetUtilsJavascriptCallback");
    Q_REG(SetWalletsJavascriptCallback, "SetWalletsJavascriptCallback");
    Q_REG(SetMetaGateJavascriptCallback, "SetMetaGateJavascriptCallback");
    Q_REG(SetProxyJavascriptCallback, "SetProxyJavascriptCallback");
    Q_REG2(QUrl, "QUrl", false);


    WebUrlRequestInterceptor *wuri = new WebUrlRequestInterceptor(this);
    QWebEngineProfile::defaultProfile()->setRequestInterceptor(wuri);

    TorUrlSchemeHandler *torShemeHandler = new TorUrlSchemeHandler(this);
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(QByteArrayLiteral("tor"), torShemeHandler);
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(QByteArrayLiteral("tors"), torShemeHandler);
    Q_CONNECT(&torProxy, &tor::TorProxy::torProxyStarted, torShemeHandler, &TorUrlSchemeHandler::setProxy);

    shemeHandler = new MHUrlSchemeHandler(this);
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(QByteArray("mh"), shemeHandler);

    channel = std::make_unique<QWebChannel>(ui->webView);
    ui->webView->page()->setWebChannel(channel.get());
    registerWebChannel(QString("initializer"), &initializerJs);

    hardwareId = QString::fromStdString(::getMachineUid());

    configureMenu();

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("dns/metahash"), "dns/metahash server not found");
    urlDns = settings.value("dns/metahash").toString();
    CHECK(settings.contains("dns/net"), "dns/net server not found");
    netDns = settings.value("dns/net").toString();

    loadPagesMappings();

    loadFile("core/loader/index.html");

    client.setParent(this);
    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &MainWindow::onCallbackCall);

    Q_CONNECT(&initializerJs, &initializer::InitializerJavascript::jsRunSig, this, &MainWindow::onJsRun);

    ui->webView->setContextMenuPolicy(Qt::CustomContextMenu);
    Q_CONNECT(ui->webView, &QWebEngineView::customContextMenuRequested, this, &MainWindow::onShowContextMenu);

    //Q_CONNECT(ui->webView->page(), &QWebEnginePage::loadFinished, this, &MainWindow::onBrowserLoadFinished);

    Q_CONNECT(ui->webView, &WebView::urlChanged, this, &MainWindow::onUrlChanged);

    correctWindowSize(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::correctWindowSize(int iteratiion) {
    const QRect rec = QApplication::desktop()->availableGeometry();
    const QSize screenSize(rec.width(), rec.height());

    const QRect geometry = this->geometry();
    const QSize windowSize = geometry.size();
    const QSize windowSizeFrame = this->frameGeometry().size();
    if (windowSize == windowSizeFrame && iteratiion <= 10) {
        QTimer::singleShot(200, std::bind(&MainWindow::correctWindowSize, this, iteratiion + 1));
        return;
    }
    LOG << "Screen size " << screenSize.width() << " " << screenSize.height() << " " << windowSizeFrame.width() << " " << windowSizeFrame.height();
    if (screenSize.width() >= windowSizeFrame.width() && screenSize.height() >= windowSizeFrame.height()) {
        return;
    }
    const QSize correct(std::max(0, windowSizeFrame.width() - screenSize.width()), std::max(0, windowSizeFrame.height() - screenSize.height()));

    ui->webView->setMinimumSize(ui->webView->minimumSize() - correct);
    ui->webView->resize(ui->webView->size() - correct);
    ui->centralWidget->setMinimumSize(ui->centralWidget->minimumSize() - correct);
    ui->centralWidget->resize(ui->centralWidget->size() - correct);
    this->resize(windowSize - correct);

    ui->webView->adjustSize();
    ui->centralWidget->adjustSize();
    this->adjustSize();

    ui->webView->updateGeometry();
    ui->centralWidget->updateGeometry();
    this->updateGeometry();

    if (this->isVisible()) {
        this->showFullScreen();
        this->showNormal();
    }
}

void MainWindow::loadPagesMappings() {
    pagesMappings.clearMappings();
    pagesMappings.setFullPagesPath(last_htmls.fullPath);
    const QString routesFile = makePath(last_htmls.fullPath, "core/routes.json");
    if (isExistFile(routesFile)) {
        const std::string contentMappings = readFile(makePath(last_htmls.fullPath, "core/routes.json"));
        LOG << "Set mappings from file " << contentMappings.size();
        try {
            pagesMappings.setMappings(QString::fromStdString(contentMappings));
        } catch (const Exception &e) {
            LOG << "Error " << e;
        } catch (const TypedException &e) {
            LOG << "Error " << e.description;
        } catch (...) {
            LOG << "Error mappings";
        }
    } else {
        LOG << "Warning: routes file not found";
    }
}

void MainWindow::registerWebChannel(const QString &name, QObject *obj) {
    channel->registerObject(name, obj);
    registeredWebChannels.emplace_back(name, obj);
}

void MainWindow::unregisterAllWebChannels() {
    if (!ui->webView->page()->webChannel()) {
        return;
    }
    LOG << "Unregister all channels";
    ui->webView->page()->setWebChannel(nullptr);

    //for (const auto &pair: registeredWebChannels) {
    //    channel->deregisterObject(pair.second);
    //}
    //isRegisteredWebChannels = false;
}

void MainWindow::registerAllWebChannels() {
    //ui->webView->page()->setWebChannel(nullptr);
    if (ui->webView->page()->webChannel()) {
        return;
    }
    LOG << "Register all channels";
    ui->webView->page()->setWebChannel(channel.get());
//    for (const auto &pair: registeredWebChannels) {
//        channel->registerObject(pair.first, pair.second);
//    }
    //isRegisteredWebChannels = true;
}

void MainWindow::onSetJavascriptWrapper(JavascriptWrapper *jsWrapper1, const SetJavascriptWrapperCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(jsWrapper1 != nullptr, "Incorrect jsWrapper1");
        jsWrapper1->setWidget(this);
        Q_CONNECT(jsWrapper1, &JavascriptWrapper::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("mainWindow"), jsWrapper1);
        Q_CONNECT(jsWrapper1, &JavascriptWrapper::setCommandLineTextSig, this, &MainWindow::onSetCommandLineText);
        Q_CONNECT(jsWrapper1, &JavascriptWrapper::setMappingsSig, this, &MainWindow::onSetMappings);
        doConfigureMenu();
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetAuth(auth::AuthJavascript *authJavascript, auth::Auth *authManager, const SetAuthCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(authJavascript != nullptr, "Incorrect authJavascript");
        Q_CONNECT(authJavascript, &auth::AuthJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("auth"), authJavascript);

        CHECK(authManager != nullptr, "Incorrect authManager");
        Q_CONNECT(authManager, &auth::Auth::logined, this, &MainWindow::onLogined);
        emit authManager->reEmit();
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetMessengerJavascript(messenger::MessengerJavascript *messengerJavascript, const SetMessengerJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(messengerJavascript != nullptr, "Incorrect messengerJavascript");
        Q_CONNECT(messengerJavascript, &messenger::MessengerJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("messenger"), messengerJavascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetTransactionsJavascript(transactions::TransactionsJavascript *transactionsJavascript, const SetTransactionsJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(transactionsJavascript != nullptr, "Incorrect transactionsJavascript");
        Q_CONNECT(transactionsJavascript, &transactions::TransactionsJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("transactions"), transactionsJavascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetWalletNamesJavascript(wallet_names::WalletNamesJavascript *walletNamesJavascript, const SetWalletNamesJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(walletNamesJavascript != nullptr, "Incorrect proxyJavascript");
        Q_CONNECT(walletNamesJavascript, &wallet_names::WalletNamesJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("wallet_names"), walletNamesJavascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetUtilsJavascript(utils::UtilsJavascript *utilsJavascript, const SetUtilsJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(utilsJavascript != nullptr, "Incorrect utilsJavascript");
        Q_CONNECT(utilsJavascript, &utils::UtilsJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("utils"), utilsJavascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetWalletsJavascript(wallets::WalletsJavascript *javascript, const SetWalletsJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(javascript != nullptr, "Incorrect walletsJavascript");
        Q_CONNECT(javascript, &wallets::WalletsJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("wallets"), javascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetMetaGateJavascript(metagate::MetaGate *manager, metagate::MetaGateJavascript *javascript, const SetMetaGateJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(javascript != nullptr, "Incorrect metagateJavascript");
        CHECK(manager != nullptr, "Incorrect metagate");
        metagate = manager;
        Q_CONNECT(javascript, &metagate::MetaGateJavascript::jsRunSig, this, &MainWindow::onJsRun);
        Q_CONNECT(manager, &metagate::MetaGate::lineEditReturnPressedSig, this, &MainWindow::onEnterCommandAndAddToHistory);
        registerWebChannel(QString("metagate"), javascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onSetProxyJavascript(proxy_client::ProxyClientJavascript *javascript, const SetProxyJavascriptCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        CHECK(javascript != nullptr, "Incorrect proxyJavascript");
        Q_CONNECT(javascript, &proxy_client::ProxyClientJavascript::jsRunSig, this, &MainWindow::onJsRun);
        registerWebChannel(QString("proxy"), javascript);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void MainWindow::onInitFinished() {
BEGIN_SLOT_WRAPPER
    isInitFinished = true;
    if (currentUserName == DEFAULT_USERNAME) {
        enterCommandAndAddToHistory("app://Login", true, true);
    } else {
        enterCommandAndAddToHistory("app://MetaApps", true, true);
    }
    ui->grid_layout->show();
    if (saveUrlToMove != QUrl()) {
        enterCommandAndAddToHistory(saveUrlToMove.toString(), true, true);
    }
END_SLOT_WRAPPER
}

void MainWindow::onProcessExternalUrl(const QUrl &url) {
BEGIN_SLOT_WRAPPER
    if (isInitFinished) {
        enterCommandAndAddToHistory(url.toString(), false, true);
    } else {
        saveUrlToMove = url;
    }
END_SLOT_WRAPPER
}

void MainWindow::onShowNotification(const QString &title, const QString &message)
{
BEGIN_SLOT_WRAPPER
    CHECK(systemTray, "systemTray error");
    LOG << "Notification: " << title << " " << message;
    systemTray->showMessage(title, message, QSystemTrayIcon::Information, 5000);
END_SLOT_WRAPPER
}

void MainWindow::onCallbackCall(SimpleClient::ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void MainWindow::doConfigureMenu() {
    CHECK(metagate != nullptr, "jsWrapper not set");
    Q_CONNECT3(ui->commandLine->lineEdit(), &QLineEdit::editingFinished, [this]{
        countFocusLineEditChanged++;
    });
    Q_CONNECT3(ui->commandLine->lineEdit(), &QLineEdit::textEdited, [this](const QString &text){
        qDebug() << "EDIT:" << text;
        emit urlChanged(text);
        lineEditUserChanged = true;
        emit metagate->sendCommandLineMessageToWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, text, false, true);
    });
    Q_CONNECT3(ui->commandLine->lineEdit(), &QLineEdit::textChanged, [this](const QString &text){
        if (!lineEditUserChanged) {
            emit metagate->sendCommandLineMessageToWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, text, false, false);
        }
    });
    Q_CONNECT3(ui->commandLine->lineEdit(), &QLineEdit::returnPressed, [this]{
        emit urlEntered(ui->commandLine->lineEdit()->text());
        emit metagate->sendCommandLineMessageToWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, ui->commandLine->lineEdit()->text(), true, true);
        ui->commandLine->lineEdit()->setText(currentTextCommandLine);
    });
    Q_CONNECT3(ui->commandLine->lineEdit(), &QLineEdit::editingFinished, [this](){
        lineEditUserChanged = false;
    });
}

void MainWindow::configureMenu() {
    ui->grid_layout->hide();

    this->setStyleSheet("QMainWindow {background: rgb(242,242,242);}");

    QFontDatabase::addApplicationFont(":/resources/Roboto-Regular.ttf");
#ifdef TARGET_OS_MAC
    const int fontSize = 12;
#else
    const int fontSize = 10;
#endif
    QFont font("Roboto", fontSize);
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);

    auto configureBrowserButton = [](QAbstractButton *button) {
        button->setStyleSheet(
            "QAbstractButton {background-color: transparent; border-radius: 5px;} "
            "QAbstractButton:hover { background-color: #e6e6e6; border-radius: 5px;}"
        );
    };

    auto configureMenuButton = [this, &font](QAbstractButton *button, const QString &icoActive, const QString icoHover) {
        button->setFont(font);
        button->setStyleSheet(
            "QAbstractButton {color: rgb(99, 99, 99); background-color: transparent; border-radius: 5px;} "
            "QAbstractButton:hover { background-color: #4F4F4F; color: white; border-radius: 5px;} "
            "QToolButton::menu-indicator { image: none; }"
        );

        button->installEventFilter(new EvFilter(this, icoActive, icoHover));
    };

    //configureMenuButton(ui->buyButton, ":/resources/svg/Buy_MHC.svg", ":/resources/svg/Buy_MHC_white.svg");
    configureMenuButton(ui->metaAppsButton, ":/resources/svg/MetaApps.svg", ":/resources/svg/MetaApps_white.svg");
    configureMenuButton(ui->metaWalletButton, ":/resources/svg/MetaWallet.svg", ":/resources/svg/MetaWallet_white.svg");
    configureMenuButton(ui->userButton, ":/resources/svg/user.svg", ":/resources/svg/user_white.svg");

    configureBrowserButton(ui->backButton);
    configureBrowserButton(ui->forwardButton);
    configureBrowserButton(ui->refreshButton);

    QFont fontCommandLine("Roboto", 12);
    fontCommandLine.setBold(false);
    fontCommandLine.setItalic(false);
    fontCommandLine.setKerning(false);

    ui->commandLine->setFont(fontCommandLine);
    ui->commandLine->setStyleSheet(
        "QComboBox {color: rgb(99, 99, 99); border-radius: 14px; padding-left: 14px; padding-right: 14px; } "
        "QComboBox::drop-down {padding-top: 10px; padding-right: 10px; width: 10px; height: 10px; image: url(:/resources/svg/arrow.svg);}"
    );
    ui->commandLine->setAttribute(Qt::WA_MacShowFocusRect, 0);

    registerCommandLine();

    Q_CONNECT3(ui->backButton, &QToolButton::pressed, [this] {
        BEGIN_SLOT_WRAPPER
        historyPos--;
        enterCommandAndAddToHistory(history.at(historyPos - 1), false, false);
        ui->backButton->setEnabled(historyPos > 1);
        ui->forwardButton->setEnabled(historyPos < history.size());
        END_SLOT_WRAPPER;
    });
    ui->backButton->setEnabled(false);

    Q_CONNECT3(ui->forwardButton, &QToolButton::pressed, [this]{
        BEGIN_SLOT_WRAPPER
        historyPos++;
        enterCommandAndAddToHistory(history.at(historyPos - 1), false, false);
        ui->backButton->setEnabled(historyPos > 1);
        ui->forwardButton->setEnabled(historyPos < history.size());
        END_SLOT_WRAPPER
    });
    ui->forwardButton->setEnabled(false);

    Q_CONNECT(ui->refreshButton, &QToolButton::pressed, ui->webView, &QWebEngineView::reload);

    Q_CONNECT3(ui->userButton, &QAbstractButton::pressed, [this]{
        BEGIN_SLOT_WRAPPER
        onEnterCommandAndAddToHistory("Settings");
        END_SLOT_WRAPPER
    });

    /*Q_CONNECT3(ui->buyButton, &QAbstractButton::pressed, [this]{
        onEnterCommandAndAddToHistory("BuyMHC");
    });*/

    Q_CONNECT3(ui->metaWalletButton, &QAbstractButton::pressed, [this]{
        BEGIN_SLOT_WRAPPER
        onEnterCommandAndAddToHistory("Wallet");
        END_SLOT_WRAPPER
    });

    Q_CONNECT3(ui->metaAppsButton, &QAbstractButton::pressed, [this]{
        BEGIN_SLOT_WRAPPER
        onEnterCommandAndAddToHistory("MetaApps");
        END_SLOT_WRAPPER
    });
}

void MainWindow::registerCommandLine()
{
    Q_CONNECT(ui->commandLine, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onEnterCommandAndAddToHistoryNoDuplicate);
}

void MainWindow::unregisterCommandLine()
{
    CHECK(disconnect(ui->commandLine, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onEnterCommandAndAddToHistoryNoDuplicate), "not disconnect currentIndexChanged");
}

void MainWindow::registerUrlChangedHandler()
{
    Q_CONNECT(ui->webView, &WebView::urlChanged, this, &MainWindow::onUrlChanged);

}

void MainWindow::unregisterUrlChangedHandler()
{
    disconnect(ui->webView, &WebView::urlChanged, this, &MainWindow::onUrlChanged);
}

void MainWindow::onEnterCommandAndAddToHistory(const QString &text)
{
BEGIN_SLOT_WRAPPER
    enterCommandAndAddToHistory(text, true, false);
END_SLOT_WRAPPER
}

void MainWindow::onEnterCommandAndAddToHistoryNoDuplicate(const QString &text)
{
BEGIN_SLOT_WRAPPER
    enterCommandAndAddToHistory(text, true, true);
END_SLOT_WRAPPER
}

void MainWindow::enterCommandAndAddToHistory(const QString &text1, bool isAddToHistory, bool isNoEnterDuplicate)
{
    LOG << "command line " << text1;

    const static QString HTTP_1_PREFIX = QLatin1String("http://");
    const static QString HTTP_2_PREFIX = QLatin1String("https://");

    const static QString TOR_1_PREFIX = QLatin1String("tor://");
    const static QString TOR_2_PREFIX = QLatin1String("tors://");
    const static QLatin1String  SEARCH_PREFIX = QLatin1String("app://Search:");

    QString text = text1;
    if (text.endsWith('/')) {
        text = text.left(text.size() - 1);
    }

    if (text.startsWith(SEARCH_PREFIX)) {
        text = text.mid(SEARCH_PREFIX.size());
    }

    if (isNoEnterDuplicate && text == currentTextCommandLine) {
        return;
    }

/*    if (text.startsWith("tor://") || text.startsWith("http://")) {
        loadUrl(text);
        return;
    }
*/
    const auto doProcessCommand = [this, isAddToHistory, text](const PageInfo &pageInfo) {
        const QString &reference = pageInfo.page;

        if (reference.isNull() || reference.isEmpty()) {
            QString text2 = text;
            if (text2.startsWith(APP_URL)) {
                text2 = text2.mid(APP_URL.size());
            }
            //QTextDocument td;
            //td.setHtml(text2);
            //const QString plained = td.toPlainText();
            const PageInfo &searchPage = pagesMappings.getSearchPage();
            QString link = searchPage.page;
            qDebug() << link;
            link += QString::fromLatin1(QUrl::toPercentEncoding(text2));
            LOG << "Search page " << link;
            addElementToHistoryAndCommandLine(searchPage.printedName + ":" + text2, isAddToHistory, true);
            registerAllWebChannels();
            loadFile(link);
        } else if (reference.startsWith(METAHASH_URL)) {
            QString uri = reference.mid(METAHASH_URL.size());
            const int pos = uri.indexOf('/');
            QString other;
            if (pos != -1) {
                other = uri.mid(pos);
                uri = uri.left(pos);
            }

            QString clText;
            if (pageInfo.printedName.isNull() || pageInfo.printedName.isEmpty()) {
                clText = reference;
            } else {
                clText = pageInfo.printedName + other;
            }
            addElementToHistoryAndCommandLine(clText, isAddToHistory, true);
            unregisterAllWebChannels();
            loadUrl(reference);
        } else {
            QString clText;
            if (pageInfo.printedName.isNull() || pageInfo.printedName.isEmpty()) {
                clText = text;
            } else {
                clText = pageInfo.printedName;
            }

            if (pageInfo.isExternal) {
                qtOpenInBrowser(reference);
            } else if (reference.startsWith(HTTP_1_PREFIX) || reference.startsWith(HTTP_2_PREFIX) || !pageInfo.isLocalFile) {
                addElementToHistoryAndCommandLine(clText, isAddToHistory, true);
                const QUrl url(reference);
                if (url.host() == QLatin1String("localhost") || url.host() == QLatin1String("127.0.0.1")) {
                    registerAllWebChannels();
                } else {
                    unregisterAllWebChannels();
                }
                loadUrl(reference);
            } else {
                addElementToHistoryAndCommandLine(clText, isAddToHistory, true);
                registerAllWebChannels();
                unregisterUrlChangedHandler();
                loadFile(reference);
                registerUrlChangedHandler();
            }
        }
    };

    const PageInfo pageInfo = pagesMappings.find(text);
    if (pageInfo.isApp || pageInfo.isRedirectShemeHandler) {
        doProcessCommand(pageInfo);
        return;
    } else if (text.startsWith(HTTP_1_PREFIX) || text.startsWith(HTTP_2_PREFIX) ||
               text.startsWith(TOR_1_PREFIX) || text.startsWith(TOR_2_PREFIX)) {
        addElementToHistoryAndCommandLine(text, isAddToHistory, true);
        const QUrl url(text);
        if (url.host() == QLatin1String("localhost") || url.host() == QLatin1String("127.0.0.1")) {
            registerAllWebChannels();
        } else {
            unregisterAllWebChannels();
        }
        loadUrl(text);
    } else {
        addElementToHistoryAndCommandLine(text, isAddToHistory, true);
        const QString host = PagesMappings::getHost(text);
        const QString postRequest = "{\"id\":1, \"method\":\"custom\", \"params\":{\"name\": \"" + host + "\", \"net\": \"" + netDns + "\"}}";
        client.sendMessagePost(urlDns, postRequest, [this, text, doProcessCommand](const SimpleClient::Response &response) {
            CHECK(!response.exception.isSet(), "Dns error " + response.exception.toString());
            qDebug() << QString::fromStdString(response.response);
            pagesMappings.addMappingsMh(QString::fromStdString(response.response));
            const PageInfo pageInfo = pagesMappings.find(text);
            doProcessCommand(pageInfo);
        }, 2s);
    }
}

void MainWindow::qtOpenInBrowser(QString url) {
    LOG << "Open url in default browser " << url;
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::onShowContextMenu(const QPoint &point) {
BEGIN_SLOT_WRAPPER
    QMenu contextMenu(tr("Context menu"), this);

    contextMenu.addAction("cut", []{
        QWidget* focused = QApplication::focusWidget();
        if(focused != 0) {
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_X, Qt::ControlModifier));
        }
    });

    contextMenu.addAction("copy", []{
        QWidget* focused = QApplication::focusWidget();
        if(focused != 0) {
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier));
        }
    });

    contextMenu.addAction("paste", []{
        QWidget* focused = QApplication::focusWidget();
        if(focused != 0) {
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier));
        }
    });

    contextMenu.exec(mapToGlobal(point));
END_SLOT_WRAPPER
}

void MainWindow::updateHtmlsEvent() {
BEGIN_SLOT_WRAPPER
    showNotification(tr("Web application updated"), tr("Restart MetaGate"));
    softReloadPage();
END_SLOT_WRAPPER
}

void MainWindow::updateAppEvent(const QString appVersion, const QString reference, const QString message) {
BEGIN_SLOT_WRAPPER
    const QString currentVersion = VERSION_STRING;
    const QString jsScript = "window.onQtAppUpdate  && window.onQtAppUpdate(\"" + appVersion + "\", \"" + reference + "\", \"" + currentVersion + "\", \"" + message + "\");";
    LOG << "Update script " << jsScript;
    showNotification(tr("New %1 update(s) are available").arg(reference), tr("Current MetaGate version %1").arg(currentVersion));
    ui->webView->page()->runJavaScript(jsScript);
END_SLOT_WRAPPER
}

void MainWindow::softReloadPage() {
    LOG << "updateReady()";
    if (!isInitFinished) {
        std::unique_lock<std::mutex> lock(mutLastHtmls);
        last_htmls = Uploader::getLastHtmlVersion();
        loadPagesMappings();
        lock.unlock();
        loadFile("core/loader/index.html");
    } else {
        ui->webView->page()->runJavaScript("updateReady();");
    }
}

void MainWindow::softReloadApp() {
    QApplication::exit(RESTART_BROWSER);
}

void MainWindow::loadUrl(const QString &page) {
    LOG << "Reload. " << page << ".";
    shemeHandler->setLog();
    shemeHandler->setFirstRun();
    QUrl url(page);
    if (url.path().isEmpty()) {
        url.setPath("/");
    }
    ui->webView->load(url);
    LOG << "Reload ok";
}

void MainWindow::loadFile(const QString &pageName) {
    loadUrl("file:///" + makePath(last_htmls.fullPath, pageName));
}

bool MainWindow::currentFileIsEqual(const QString &pageName) {
    return ui->webView->url().toString().endsWith(makePath(last_htmls.fullPath, pageName));
}

void MainWindow::addElementToHistoryAndCommandLine(const QString &text, bool isAddToHistory, bool isReplace) {
    LOG << "scl " << isAddToHistory << " " << text;

    unregisterCommandLine();
    const QString currText = ui->commandLine->currentText();
    if (!currText.isEmpty()) {
        if (ui->commandLine->count() >= 1) {
            if (!pagesMappings.compareTwoPaths(ui->commandLine->itemText(ui->commandLine->count() - 1), currText)) {
                ui->commandLine->addItem(currText);
            }
        } else {
            ui->commandLine->addItem(currText);
        }
    }

    bool isSetText = true;
    if (pagesMappings.compareTwoPaths(currText, text)) {
        isSetText = isReplace;
    }
    if (isSetText) {
        ui->commandLine->setCurrentText(text);
    }

    currentTextCommandLine = text;

    if (isAddToHistory && isSetText) {
        if (historyPos == 0 || !pagesMappings.compareTwoPaths(history[historyPos - 1], text)) {
            history.insert(history.begin() + historyPos, text);
            historyPos++;

            ui->backButton->setEnabled(history.size() > 1);
        } else if (pagesMappings.compareTwoPaths(history[historyPos - 1], text)) {
            history.at(historyPos - 1) = text;
        }
    }

    registerCommandLine();
}

void MainWindow::onUrlChanged(const QUrl &url2) {
BEGIN_SLOT_WRAPPER
    qDebug() << "URL new " << url2;
    const QString url = url2.toString();

//////////////////// Hack for metapay internal redirects
    if (url2.scheme() == QLatin1String("metapay")) {
        const PageInfo pageInfo = pagesMappings.find(url);

        const QString &reference = pageInfo.page;

            QString clText;
            if (pageInfo.printedName.isNull() || pageInfo.printedName.isEmpty()) {
                clText = url;
            } else {
                clText = pageInfo.printedName;
            }

                addElementToHistoryAndCommandLine(clText, false, true);
                registerAllWebChannels();
                loadFile(reference);

        return;
    }
////////////////////
    const QString turl = QUrl::fromPercentEncoding(url.toUtf8());
    //const QString url = url2.toDisplayString(QUrl::None);

    const Optional<PageInfo> found = pagesMappings.findName(url);
    if (found.has_value()) {
        LOG << "Set address after load " << found.value().printedName;
        if (!found.value().isApp) {
            unregisterAllWebChannels();
        } else {
            registerAllWebChannels();
        }
        prevIsApp = found.value().isApp;
        addElementToHistoryAndCommandLine(found.value().printedName, true, false);
    } else {
        if (url2.scheme() == QLatin1String("http") || url2.scheme() == QLatin1String("https")) {
            unregisterAllWebChannels();
            addElementToHistoryAndCommandLine(url, true, false);
        } else if (!prevUrl.isNull() && !prevUrl.isEmpty() && turl.startsWith(prevUrl)) {
            const QString request = turl.mid(prevUrl.size());
            LOG << "Set address after load2 " << prevTextCommandLine << " " << request << " " << prevUrl;
            if (!prevIsApp) {
                unregisterAllWebChannels();
            }
            addElementToHistoryAndCommandLine(prevTextCommandLine + request, true, false);
        } else {
            LOG << "not set address after load " << url << " " << currentTextCommandLine;
        }
    }
    prevUrl = url;
    prevTextCommandLine = currentTextCommandLine;
END_SLOT_WRAPPER
}

void MainWindow::onSetCommandLineText(QString text) {
BEGIN_SLOT_WRAPPER
    addElementToHistoryAndCommandLine(text, true, true);
END_SLOT_WRAPPER
}

void MainWindow::setUserName(QString userName) {
    currentUserName = userName;
    LOG << "Set user name " << userName;
    ui->userButton->setText(userName);
    ui->userButton->adjustSize();

    auto *button = ui->userButton;
    const auto textSize = button->fontMetrics().size(button->font().style(), button->text());
    QStyleOptionButton opt;
    opt.initFrom(button);
    opt.rect.setSize(textSize);
    const int estimatedWidth = button->style()->sizeFromContents(QStyle::CT_ToolButton, &opt, textSize, button).width() + 25;
    button->setMaximumWidth(estimatedWidth);
    button->setMinimumWidth(estimatedWidth);
}

void MainWindow::onSetMappings(QString mapping) {
BEGIN_SLOT_WRAPPER
    LOG << PeriodicLog::make("s_mp") << "Set mappings from site " << mapping;
    pagesMappings.setMappings(mapping);
END_SLOT_WRAPPER
}

void MainWindow::onJsRun(QString jsString) {
BEGIN_SLOT_WRAPPER
    //if (isRegisteredWebChannels) {
    if (ui->webView->page()->webChannel()) {
        ui->webView->page()->runJavaScript(jsString);
    } else {
        LOG << "Revert javascript";
    }
END_SLOT_WRAPPER
}

void MainWindow::showExpanded() {
    show();
}

void MainWindow::showOnTop()
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    showNormal();
    show();
    activateWindow();
    raise();
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    show();
}

QString MainWindow::getServerIp(const QString &text, const std::set<QString> &excludesIps) {
    try {
        QString ip = pagesMappings.getIp(text, excludesIps);
        QUrl url(ip);
        return url.host();
    } catch (const Exception &e) {
        LOG << "Error " << e;
        return "";
    }
}

LastHtmlVersion MainWindow::getCurrentHtmls() const {
    std::lock_guard<std::mutex> lock(mutLastHtmls);
    return last_htmls;
}

QString MainWindow::getCurrenttUrl() const
{
    return ui->commandLine->lineEdit()->text();
}

void MainWindow::setCurrentUrl(const QString &url)
{
    ui->commandLine->lineEdit()->setText(url);
}

void MainWindow::setVisible(bool visible)
{
    hideAction->setEnabled(visible);
    showAction->setEnabled(!visible);
    QMainWindow::setVisible(visible);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_OS_MACOS
    // Show window on dock clicks
    if (obj == qApp && event->type() == QEvent::ApplicationStateChange) {
        QApplicationStateChangeEvent *ev = static_cast<QApplicationStateChangeEvent *>(event);
        if (ev->applicationState() == Qt::ApplicationActive)
            showOnTop();
    }
#endif // Q_OS_MACOS
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    hide();
    hideAction->setEnabled(this->isVisible());
    event->ignore();
}

void MainWindow::changeEvent(QEvent *event)
{
#ifdef Q_OS_OSX
    if(event->type() == QEvent::WindowStateChange)
    {
        if (isMinimized() || isFullScreen()) {
            // Disable hide/show for minimized window
            hideAction->setEnabled(false);
            showAction->setEnabled(false);
        } else {
            hideAction->setEnabled(isVisible());
            showAction->setEnabled(!isVisible());
        }
    }
#endif
}

void MainWindow::onLogined(bool /*isInit*/, const QString &login) {
BEGIN_SLOT_WRAPPER
    if (login.isEmpty()) {
        if (isInitFinished) {
            LOG << "Try Swith to login";
            if (!currentFileIsEqual("login.html")) {
                LOG << "Swith to login";
                loadFile("login.html");
                addElementToHistoryAndCommandLine("app://Login", true, true);
            }
        }
        setUserName(DEFAULT_USERNAME);
    } else {
        setUserName(login);
    }
END_SLOT_WRAPPER
}
