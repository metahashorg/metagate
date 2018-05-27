#include "mainwindow.h"

#include <iostream>
#include <fstream>
#include <map>

#include <QWebEnginePage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextDocument>
#include <QLineEdit>
#include <QDir>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QKeyEvent>
#include <QMenu>
#include <QStandardItemModel>
#include <QFontDatabase>
#include <QWebEngineHistory>

#include "Wallet.h"
#include "EthWallet.h"
#include "BtcWallet.h"

#include "NsLookup.h"
#include "WebSocketClient.h"

#include "uploader.h"
#include "unzip.h"
#include "check.h"
#include "StopApplication.h"
#include "duration.h"
#include "Log.h"
#include "utils.h"
#include "TypedException.h"

#include "machine_uid.h"

const static QString WALLET_PREV_PATH = ".metahash_wallets/";
const static QString WALLET_PATH_DEFAULT = ".metahash_wallets/";
const static QString WALLET_PATH_ETH = "eth/";
const static QString WALLET_PATH_BTC = "btc/";
const static QString WALLET_PATH_MTH = "mhc/";
const static QString WALLET_PATH_TMH_OLD = "mth/";
const static QString WALLET_PATH_TMH = "tmh/";

const static QString METAGATE_URL = "/MetaGate/";

const static size_t INDEX_DESCRIPTION_LIST_ITEM = Qt::UserRole + 5;

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

static void createFolder(const QString &folder) {
    QDir dir(folder);
    const bool resultCreate = dir.mkpath(folder);
    CHECK(resultCreate, "dont create folder " + folder.toStdString());
}

static QString makeMessageForWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter) {
    CHECK(!hardwareId.contains(' '), "Incorrect symbol int string " + hardwareId.toStdString());
    CHECK(!userId.contains(' '), "Incorrect symbol int string " + hardwareId.toStdString());

    QJsonObject obj;
    obj.insert("machine_uid", hardwareId);
    obj.insert("user_id", userId);
    obj.insert("focus_release_count", (int)focusCount);
    obj.insert("text", QString(line.toUtf8().toHex()));
    obj.insert("is_enter_pressed", isEnter);
    QJsonDocument json(obj);

    return json.toJson(QJsonDocument::Compact);
}

MainWindow::MainWindow(ServerName &serverName, NsLookup &nsLookup, WebSocketClient &webSocketClient, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , serverName(serverName)
    , nsLookup(nsLookup)
    , webSocketClient(webSocketClient)
{
    ui->setupUi(this);

    hardwareId = QString::fromStdString(::getMachineUid());

    configureMenu();

    currentBeginPath = Uploader::getPagesPath();

    hardReloadPage("login.html");

    channel = std::make_unique<QWebChannel>(ui->webView->page());
    ui->webView->page()->setWebChannel(channel.get());
    channel->registerObject(QString("mainWindow"), this);

    walletDefaultPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath(WALLET_PATH_DEFAULT);
    LOG << "Wallets default path " << walletPath;

    ui->webView->setContextMenuPolicy(Qt::CustomContextMenu);
    CHECK(connect(ui->webView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))), "not connect");

    setPaths(walletDefaultPath, "");
}

void MainWindow::setUserName(const QString &userName) {
    ui->userButton->setText(userName);
    ui->userButton->adjustSize();

    auto *button = ui->userButton;
    const auto textSize = button->fontMetrics().size(button->font().style(), button->text());
    QStyleOptionButton opt;
    opt.initFrom(button);
    opt.rect.setSize(textSize);
    const size_t estimatedWidth = button->style()->sizeFromContents(QStyle::CT_ToolButton, &opt, textSize, button).width() + 25;
    button->setMaximumWidth(estimatedWidth);
    button->setMinimumWidth(estimatedWidth);
}

void MainWindow::configureMenu() {
    this->setStyleSheet("QMainWindow {background: rgb(242,242,242);}");

    QFontDatabase::addApplicationFont(":/resources/Roboto-Regular.ttf");

    settingsList = new QMenu(this);
    logoutMenu = new QMenu(this);
    loginMenu = new QMenu(this);

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
            "QAbstractButton:hover { background-color: #4F4F4F; color: white; border-radius: 5px;}"
            "QToolButton::menu-indicator { image: none; }"
        );

        button->installEventFilter(new EvFilter(this, icoActive, icoHover));
    };

    configureMenuButton(ui->buyButton, ":/resources/svg/Buy_MHC.svg", ":/resources/svg/Buy_MHC_white.svg");
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
    ui->commandLine->setStyleSheet("QComboBox {color: rgb(99, 99, 99); border-radius: 14px; padding-left: 14px; padding-right: 14px; } QComboBox::drop-down {padding-top: 10px; padding-right: 10px; width: 10px; height: 10px; image: url(:/resources/svg/arrow.svg);}");
    ui->commandLine->setAttribute(Qt::WA_MacShowFocusRect, 0);

    registerCommandLine();

    auto configureMenu = [this, &font](QMenu *menu) {
        menu->setFont(font);
        menu->setStyleSheet("QMenu {color: rgb(51, 122, 183); border-radius: 0; background-color: white; } QMenu::item { background-color: white; } QMenu::item:selected { background-color: #e6e6e6; } QMenu::item { height: 200%;}");
        menu->setVisible(false);
        menu->setLayoutDirection(Qt::RightToLeft);
    };

    configureMenu(settingsList);
    configureMenu(loginMenu);
    configureMenu(logoutMenu);

    CHECK(connect(ui->backButton, &QToolButton::pressed, [this] {
        historyPos--;
        lineEditReturnPressed2(history.at(historyPos - 1), false);
        ui->backButton->setEnabled(historyPos > 1);
        ui->forwardButton->setEnabled(historyPos < history.size());
    }), "not connect");
    ui->backButton->setEnabled(false);
    /*QAction *action = ui->webView->page()->action(QWebEnginePage::Back);
    CHECK(connect(action, &QAction::changed, [this, action, btn=ui->backButton]{
        if (ui->webView->history()->itemAt(0).url().toString() == "about:blank") {
            ui->webView->history()->clear();
        }
        btn->setEnabled(action->isEnabled() && ui->webView->history()->items().size() > 0);
    }), "Not connect");*/

    CHECK(connect(ui->forwardButton, &QToolButton::pressed, [this]{
        historyPos++;
        lineEditReturnPressed2(history.at(historyPos - 1), false);
        ui->forwardButton->setEnabled(historyPos < history.size());
        ui->backButton->setEnabled(historyPos > 1);
    }), "not connect");
    ui->forwardButton->setEnabled(false);
    /*QAction *action2 = ui->webView->page()->action(QWebEnginePage::Forward);
    CHECK(connect(action2, &QAction::changed, [action2, btn=ui->forwardButton]{
        btn->setEnabled(action2->isEnabled());
    }), "Not connect");*/

    CHECK(connect(ui->refreshButton, SIGNAL(pressed()), ui->webView, SLOT(reload())), "not connect");

    CHECK(connect(ui->buyButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed(METAGATE_URL + "BuyMHC");
    }), "Not connect");

    CHECK(connect(ui->metaWalletButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed(METAGATE_URL + "Wallet");
    }), "Not connect");

    CHECK(connect(ui->metaAppsButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed(METAGATE_URL + "Apps");
    }), "Not connect");

    CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::editingFinished, [this]{
        countFocusLineEditChanged++;
    }), "Not connect");

    CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::textChanged, [this](const QString &text){
        emit webSocketClient.sendMessage(makeMessageForWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, text, false));
    }), "Not connect");
    CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::returnPressed, [this]{
        emit webSocketClient.sendMessage(makeMessageForWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, ui->commandLine->lineEdit()->text(), true));
    }), "Not connect");
}

void MainWindow::registerCommandLine() {
    CHECK(connect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(lineEditReturnPressed(const QString&))), "not connect");
    /*CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::returnPressed, [this](){
        LOG << "Ya tuta " << ui->commandLine->lineEdit()->text().toStdString() << " " << currentTextCommandLine.toStdString() << std::endl;
        if (ui->commandLine->lineEdit()->text() == currentTextCommandLine) {
            LOG << "refresh" << std::endl;
            emit ui->refreshButton->pressed();
        }
    }), "not connect");*/
}

void MainWindow::unregisterCommandLine() {
    CHECK(disconnect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(lineEditReturnPressed(const QString&))), "not connect");
    //CHECK(disconnect(ui->commandLine->lineEdit(), SIGNAL(returnPressed(const QString&))), "not connect");
}

void MainWindow::lineEditReturnPressed(const QString &text) {
    lineEditReturnPressed2(text);
}

void MainWindow::lineEditReturnPressed2(const QString &text1, bool isAddToHistory) {
    const QString METAHASH_URL = "mh://";

    LOG << "command line " << text1;

    QString text = text1;
    if (text.endsWith('/')) {
        text = text.left(text.size() - 1);
    }

    if (text == currentTextCommandLine) {
        return;
    }

    auto runSearch = [&, this](const QString &url) {
        QTextDocument td;
        td.setHtml(url);
        const QString plained = td.toPlainText();
        QString link = "";
        for (const auto pageInfoIt: mappingsPages) {
            if (pageInfoIt.second.isDefault) {
                link = pageInfoIt.second.page;
            }
        }
        if (link.isNull() || link.isEmpty()) {
            LOG << "Error. Not found url " << url << " in mappings. Plained: " << plained;
            return;
        }
        link += plained;
        LOG << "Founded page " << link;
        setCommandLineText2(url, isAddToHistory);
        //currentTextCommandLine = url;
        hardReloadPage(link);
    };

    const QString url = text;
    if (url.startsWith(METAGATE_URL)) {
        const auto found = mappingsPages.find(url);
        if (found == mappingsPages.end()) {
            runSearch(url);
            return;
        }
        if (found->second.isExternal) {
            qtOpenInBrowser(found->second.page);
            return;
        } else {
            setCommandLineText2(text, isAddToHistory);
            hardReloadPage(found->second.page);
        }
    } else if (url.startsWith(METAHASH_URL)) {
        QString uri = url.mid(METAHASH_URL.size());
        const size_t pos1 = uri.indexOf('/');
        const size_t pos2 = uri.indexOf('?');
        const size_t min = std::min(pos1, pos2);
        QString other;
        if (min != size_t(-1)) {
            other = uri.mid(min);
            uri = uri.left(min);
        }

        LOG << "switch to url " << uri;
        LOG << "other " << other;
        QWebEngineHttpRequest req("http://31.172.81.6" + other);
        req.setHeader("host", uri.toUtf8());
        setCommandLineText2(text, isAddToHistory);
        hardReloadPage2(req);
    } else {
        runSearch(url);
        /*setCommandLineText(text);
        hardReloadPage2(url);*/
    }
}

void MainWindow::ShowContextMenu(const QPoint &point) {
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("cut", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(contextMenuCut()));
    contextMenu.addAction(&action1);

    QAction action2("copy", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(contextMenuCopy()));
    contextMenu.addAction(&action2);

    QAction action3("paste", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(contextMenuPaste()));
    contextMenu.addAction(&action3);

    contextMenu.exec(mapToGlobal(point));
}

void MainWindow::contextMenuCut() {
    QWidget* focused = QApplication::focusWidget();
    if(focused != 0) {
        QApplication::postEvent(
            focused,
            new QKeyEvent(
                QEvent::KeyPress,
                Qt::Key_X,
                Qt::ControlModifier
            )
        );
        QApplication::postEvent(
            focused,
            new QKeyEvent(
                QEvent::KeyRelease,
                Qt::Key_X,
                Qt::ControlModifier
            )
        );
    }
}

void MainWindow::contextMenuCopy() {
    QWidget* focused = QApplication::focusWidget();
    if(focused != 0) {
        QApplication::postEvent(
            focused,
            new QKeyEvent(
                QEvent::KeyPress,
                Qt::Key_C,
                Qt::ControlModifier
            )
        );
        QApplication::postEvent(
            focused,
            new QKeyEvent(
                QEvent::KeyRelease,
                Qt::Key_C,
                Qt::ControlModifier
            )
        );
    }
}

void MainWindow::contextMenuPaste() {
    QWidget* focused = QApplication::focusWidget();
    if(focused != 0) {
        QApplication::postEvent(
            focused,
            new QKeyEvent(
                QEvent::KeyPress,
                Qt::Key_V,
                Qt::ControlModifier
            )
        );
        QApplication::postEvent(
            focused,
            new QKeyEvent(
                QEvent::KeyRelease,
                Qt::Key_V,
                Qt::ControlModifier
            )
        );
    }
}

void MainWindow::processEvent(WindowEvent event) {
    try {
        if (event == WindowEvent::RELOAD_PAGE) {
            softReloadPage();
        }
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
}

void MainWindow::updateAppEvent(const QString appVersion, const QString reference, const QString message) {
    try {
        const QString currentVersion = VERSION_STRING;
        const QString jsScript = "window.onQtAppUpdate  && window.onQtAppUpdate(\"" + appVersion + "\", \"" + reference + "\", \"" + currentVersion + "\", \"" + message + "\");";
        LOG << "Update script " << jsScript;
        ui->webView->page()->runJavaScript(jsScript);
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
}

void MainWindow::softReloadPage() {
    LOG << "updateReady()";
    ui->webView->page()->runJavaScript("updateReady();");
}

void MainWindow::softReloadApp() {
    QApplication::exit(RESTART_BROWSER);
}

void MainWindow::hardReloadPage2(const QString &page) {
    ui->webView->page()->profile()->setRequestInterceptor(nullptr);
    ui->webView->load(page);
    LOG << "Reload ok";
}

class RequestInterceptor : public QWebEngineUrlRequestInterceptor {
public:

    explicit RequestInterceptor(QObject * parent, const QString &hostName)
        : QWebEngineUrlRequestInterceptor(parent)
        , hostName(hostName)
    {}

    virtual void interceptRequest(QWebEngineUrlRequestInfo & info) Q_DECL_OVERRIDE {
        info.setHttpHeader("host", hostName.toUtf8());
    }

private:

    const QString hostName;
};

void MainWindow::hardReloadPage2(const QWebEngineHttpRequest &url) {
    RequestInterceptor *interceptor = new RequestInterceptor(ui->webView, url.header("host"));
    ui->webView->page()->profile()->setRequestInterceptor(interceptor);

    ui->webView->load(url);
    LOG << "Reload ok";
}

void MainWindow::hardReloadPage(const QString &pageName) {
    const auto &lastVersionPair = Uploader::getLastVersion(currentBeginPath);
    const auto &folderName = lastVersionPair.first;
    const auto &lastVersion = lastVersionPair.second;

    LOG << "Reload. Last version " << lastVersion;
    ui->webView->page()->profile()->setRequestInterceptor(nullptr);
    hardReloadPage2("file:///" + QDir(QDir(QDir(currentBeginPath).filePath(folderName)).filePath(lastVersion)).filePath(pageName));
}

template<class Function>
static TypedException apiVrapper(const Function &func) {
    try {
        func();
        return TypedException(TypeErrors::NOT_ERROR, "");
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        return e;
    } catch (const Exception &e) {
        LOG << "Error " << e;
        return TypedException(TypeErrors::OTHER_ERROR, e);
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        return TypedException(TypeErrors::OTHER_ERROR, e.what());
    } catch (...) {
        LOG << "Unknown error";
        return TypedException(TypeErrors::OTHER_ERROR, "Unknown error");
    }
}

////////////////
/// METAHASH ///
////////////////

void MainWindow::createWalletMTHS(QString requestId, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Create wallet " << requestId;

    const TypedException &exception = apiVrapper([this, &jsNameResult, &requestId, &password, &walletPath]() {
        std::string publicKey;
        std::string addr;
        const std::string exampleMessage = "Example message " + std::to_string(rand());
        std::string signature;

        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createWallet(walletPath, password.toStdString(), publicKey, addr);

        publicKey.clear();
        Wallet wallet(walletPath, addr, password.toStdString());
        signature = wallet.sign(exampleMessage, publicKey);

        const QString jScript = jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(publicKey) + "\", " +
            "\"" + QString::fromStdString(addr) + "\", " +
            "\"" + QString::fromStdString(exampleMessage) + "\", " +
            "\"" + QString::fromStdString(signature) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\", " +
            "\"" + wallet.getFullPath() + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        ui->webView->page()->runJavaScript(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\", " +
            "\"" + "" + "\"" +
            ");"
        );
    }

    LOG << "Create wallet ok " << requestId;
}

void MainWindow::createWallet(QString requestId, QString password) {
    createWalletMTHS(requestId, password, walletPathTmh, "createWalletResultJs");
}

void MainWindow::createWalletMHC(QString requestId, QString password) {
    createWalletMTHS(requestId, password, walletPathMth, "createWalletMHCResultJs");
}

QString MainWindow::getAllWalletsJson() {
    return getAllMTHSWalletsJson(walletPathTmh);
}

QString MainWindow::getAllMHCWalletsJson() {
    return getAllMTHSWalletsJson(walletPathMth);
}

QString MainWindow::getAllWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(walletPathTmh);
}

QString MainWindow::getAllMHCWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(walletPathMth);
}

void MainWindow::signMessage(QString requestId, QString keyName, QString text, QString password) {
    signMessageMTHS(requestId, keyName, text, password, walletPathTmh, "signMessageResultJs");
}

void MainWindow::signMessageMHC(QString requestId, QString keyName, QString text, QString password) {
    signMessageMTHS(requestId, keyName, text, password, walletPathMth, "signMessageMHCResultJs");
}

static QString makeJsonWallets(const std::vector<std::pair<QString, QString>> &wallets) {
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        jsonArray.push_back(r.first);
    }
    QJsonDocument json(jsonArray);
    return json.toJson(QJsonDocument::Compact);
}

static QString makeJsonWalletsAndPaths(const std::vector<std::pair<QString, QString>> &wallets) {
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        QJsonObject val;
        val.insert("address", r.first);
        val.insert("path", r.second);
        jsonArray.push_back(val);
    }
    QJsonDocument json(jsonArray);
    return json.toJson(QJsonDocument::Compact);
}

QString MainWindow::getAllMTHSWalletsAndPathsJson(QString walletPath) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get mth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString MainWindow::getAllMTHSWalletsJson(QString walletPath) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWallets(result);
        LOG << "get mth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

void MainWindow::signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult) {
    LOG << requestId;
    LOG << keyName;
    LOG << text;

    const std::string textStr = text.toStdString();

    const TypedException &exception = apiVrapper([this, &jsNameResult, &requestId, &keyName, &textStr, &password, &walletPath]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        const std::string signature = wallet.sign(textStr, publicKey);

        ui->webView->page()->runJavaScript(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(signature) + "\", " +
            "\"" + QString::fromStdString(publicKey) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void MainWindow::createRsaKey(QString requestId, QString address, QString password) {
    const QString JS_NAME_RESULT = "createRsaKeyResultJs";
    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &address, &requestId, &password]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string publicKey = Wallet::createRsaKey(walletPathMth, address.toStdString(), password.toStdString());

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(publicKey) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void MainWindow::decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex) {
    const QString JS_NAME_RESULT = "decryptMessageResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string message = Wallet::decryptMessage(walletPathMth, addr.toStdString(), password.toStdString(), encryptedMessageHex.toStdString());

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(message) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

////////////////
/// ETHEREUM ///
////////////////

void MainWindow::createWalletEth(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletEthResultJs";

    LOG << "Create wallet eth " << requestId;

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &password]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = EthWallet::genPrivateKey(walletPathEth, password.toStdString());

        const QString jScript = JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(address) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\", " +
            "\"" + EthWallet::getFullPath(walletPathEth, address) + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        ui->webView->page()->runJavaScript(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\", " +
            "\"" + "" + "\"" +
            ");"
        );
    }

    LOG << "Create eth wallet ok " << requestId;
}

void MainWindow::signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data) {
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message eth";

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        EthWallet wallet(walletPathEth, address.toStdString(), password.toStdString());
        const std::string result = wallet.SignTransaction(
            nonce.toStdString(),
            gasPrice.toStdString(),
            gasLimit.toStdString(),
            to.toStdString(),
            value.toStdString(),
            data.toStdString()
        );

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(result) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

/*void MainWindow::signMessageTokensEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString contractAddress, QString to, QString value) {
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message tokens eth" << std::endl;

    const TypedException &exception = apiVrapper([&, this]() {
        const QString data = QString::fromStdString(EthWallet::makeErc20Data(value.toStdString(), to.toStdString()));
        signMessageEth(requestId, address, password, nonce, gasPrice, gasLimit, contractAddress, "0x0", data);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}*/

QString MainWindow::getAllEthWalletsJson() {
    try {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPathEth);
        const QString jsonStr = makeJsonWallets(result);
        LOG << "get eth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString MainWindow::getAllEthWalletsAndPathsJson() {
    try {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPathEth);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get eth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

///////////////
/// BITCOIN ///
///////////////

void MainWindow::createWalletBtcPswd(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId;

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &password]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = BtcWallet::genPrivateKey(walletPathBtc, password).first;

        const QString jScript = JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(address) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\", " +
            "\"" + BtcWallet::getFullPath(walletPathBtc, address) + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        ui->webView->page()->runJavaScript(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\", " +
            "\"" + "" + "\"" +
            ");"
        );
    }

    LOG << "Create btc wallet ok " << requestId;
}

void MainWindow::createWalletBtc(QString requestId) {
    createWalletBtcPswd(requestId, "");
}

void MainWindow::signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
    const QString JS_NAME_RESULT = "signMessageBtcResultJs";

    LOG << "Sign message btc";

    const TypedException &exception = apiVrapper([&, this]() {
        std::vector<BtcInput> btcInputs;

        const QJsonDocument document = QJsonDocument::fromJson(jsonInputs.toUtf8());
        CHECK(document.isArray(), "jsonInputs not array");
        const QJsonArray root = document.array();
        for (const auto &jsonObj2: root) {
            const QJsonObject jsonObj = jsonObj2.toObject();
            BtcInput input;
            CHECK(jsonObj.contains("value") && jsonObj.value("value").isString(), "value field not found");
            input.outBalance = std::stoull(jsonObj.value("value").toString().toStdString());
            CHECK(jsonObj.contains("scriptPubKey") && jsonObj.value("scriptPubKey").isString(), "scriptPubKey field not found");
            input.scriptPubkey = jsonObj.value("scriptPubKey").toString().toStdString();
            CHECK(jsonObj.contains("tx_index") && jsonObj.value("tx_index").isDouble(), "tx_index field not found");
            input.spendoutnum = jsonObj.value("tx_index").toInt();
            CHECK(jsonObj.contains("tx_hash") && jsonObj.value("tx_hash").isString(), "tx_hash field not found");
            input.spendtxid = jsonObj.value("tx_hash").toString().toStdString();
            btcInputs.emplace_back(input);
        }

        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        BtcWallet wallet(walletPathBtc, address.toStdString(), password);
        size_t estimateComissionInSatoshiInt = 0;
        if (!estimateComissionInSatoshi.isEmpty()) {
            CHECK(isDecimal(estimateComissionInSatoshi.toStdString()), "Not hex number value");
            estimateComissionInSatoshiInt = std::stoll(estimateComissionInSatoshi.toStdString());
        }
        const std::string result = wallet.buildTransaction(btcInputs, estimateComissionInSatoshiInt, value.toStdString(), fees.toStdString(), toAddress.toStdString());

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(result) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void MainWindow::signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
    signMessageBtcPswd(requestId, address, "", jsonInputs, toAddress, value, estimateComissionInSatoshi, fees);
}

QString MainWindow::getAllBtcWalletsJson() {
    try {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPathBtc);
        const QString jsonStr = makeJsonWallets(result);
        LOG << "get btc wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString MainWindow::getAllBtcWalletsAndPathsJson() {
    try {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPathBtc);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get btc wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

//////////////
/// COMMON ///
//////////////

void MainWindow::updateAndReloadApplication() {
    const QString JS_NAME_RESULT = "reloadApplicationJs";

    LOG << "Reload application ";

    const TypedException &exception = apiVrapper([&, this]() {
        updateAndRestart();

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + "Ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + "Not ok" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void MainWindow::qtOpenInBrowser(QString url) {
    LOG << "Open another url " << url;
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::getWalletFolders() {
    LOG << "getWalletFolders ";
    const QString JS_NAME_RESULT = "walletFoldersJs";
    ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
        "\"" + walletDefaultPath + "\", " +
        "\"" + walletPath + "\", " +
        "\"" + userName + "\", " +
        QString::fromStdString(std::to_string(0)) + ", " +
        "\"" + QString::fromStdString("") + "\"" +
        ");"
    );
}

bool MainWindow::migrateKeysToPath(QString newPath) {
    LOG << "Migrate keys to path " << newPath;

    const QString prevPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath(WALLET_PREV_PATH);

    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_ETH), QDir(newPath).filePath(WALLET_PATH_ETH), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_BTC), QDir(newPath).filePath(WALLET_PATH_BTC), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_MTH), QDir(newPath).filePath(WALLET_PATH_MTH), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_TMH), QDir(newPath).filePath(WALLET_PATH_TMH), false);
    copyRecursively(prevPath, QDir(newPath).filePath(WALLET_PATH_TMH), false);

    return true;
}

void MainWindow::setPaths(QString newPatch, QString newUserName) {
    const QString JS_NAME_RESULT = "setPathsJs";

    const TypedException &exception = apiVrapper([&, this]() {
        userName = newUserName;
        walletPath = newPatch;
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        createFolder(walletPath);
        walletPathEth = QDir(walletPath).filePath(WALLET_PATH_ETH);
        createFolder(walletPathEth);
        walletPathBtc = QDir(walletPath).filePath(WALLET_PATH_BTC);
        createFolder(walletPathBtc);
        walletPathMth = QDir(walletPath).filePath(WALLET_PATH_MTH);
        createFolder(walletPathMth);
        walletPathTmh = QDir(walletPath).filePath(WALLET_PATH_TMH);
        createFolder(walletPathTmh);
        walletPathOldTmh = QDir(walletPath).filePath(WALLET_PATH_TMH_OLD);
        LOG << "Wallets path " << walletPath;

        QDir oldTmhPath(walletPathOldTmh);
        if (oldTmhPath.exists()) {
            copyRecursively(walletPathOldTmh, walletPathTmh, true);
            oldTmhPath.removeRecursively();
        }

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + "Ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + "Not ok" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

QString MainWindow::openFolderDialog(QString beginPath, QString caption) {
    const QString dir = QFileDialog::getExistingDirectory(this, caption, beginPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    LOG << "choised dir " << dir;
    return dir;
}

void MainWindow::exitApplication() {
    QApplication::exit(SIMPLE_EXIT);
}

QString MainWindow::backupKeys(QString caption) {
    try {
        const QString beginPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath("backup.zip");
        const QString file = QFileDialog::getSaveFileName(this, caption, beginPath);
        ::backupKeys(walletPath, file);
        return "";
    } catch (const Exception &e) {
        return QString::fromStdString(e);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown error";
    }
}

QString MainWindow::restoreKeys(QString caption) {
    try {
        const QString beginPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath("backup.zip");
        const QString file = QFileDialog::getOpenFileName(this, caption, beginPath, "*.zip;;*.*");
        const std::string text = checkBackupFile(file);
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "caption", "Restore backup " + QString::fromStdString(text) + "?", QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            ::restoreKeys(file, walletPath);
        }
        return "";
    } catch (const Exception &e) {
        return QString::fromStdString(e);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown error";
    }
}

void MainWindow::getMachineUid() {
    const QString JS_NAME_RESULT = "machineUidJs";

    const QString uid = "\"" + hardwareId + "\"";
    ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
        uid + "" +
        ");"
    );
}

void MainWindow::setHasNativeToolbarVariable() {
    ui->webView->page()->runJavaScript("window.hasNativeToolbar = true;");
}

void MainWindow::setCommandLineText2(const QString &text, bool isAddToHistory) {
    LOG << "scl " << text;

    unregisterCommandLine();
    /*ui->commandLine->setItemText(ui->commandLine->currentIndex(), text);
    ui->commandLine->addItem(text);*/
    const QString currText = ui->commandLine->currentText();
    if (!currText.isEmpty()) {
        if (ui->commandLine->count() >= 1) {
            if (ui->commandLine->itemText(ui->commandLine->count() - 1) != currText) {
                ui->commandLine->addItem(currText);
            }
        } else {
            ui->commandLine->addItem(currText);
        }
    }
    ui->commandLine->setCurrentText(text);
    //ui->commandLine->addItem(text);

    currentTextCommandLine = text;

    if (isAddToHistory) {
        if (historyPos == 0 || history[historyPos - 1] != text) {
            history.insert(history.begin() + historyPos, text);
            historyPos++;

            ui->backButton->setEnabled(history.size() > 1);
        }
    }

    registerCommandLine();
}

void MainWindow::setCommandLineText(const QString &text) {
    setCommandLineText2(text);
}

void MainWindow::openWalletPathInStandartExplorer() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(walletPath));
}

void MainWindow::setPagesMapping(QString mapping) {
    try {
        LOG << "Set mappings " << mapping;

        const QJsonDocument document = QJsonDocument::fromJson(mapping.toUtf8());
        const QJsonObject root = document.object();
        CHECK(root.contains("routes") && root.value("routes").isArray(), "routes field not found");
        const QJsonArray &routes = root.value("routes").toArray();
        for (const QJsonValue &value: routes) {
            CHECK(value.isObject(), "value array incorrect type");
            const QJsonObject &element = value.toObject();
            CHECK(element.contains("url") && element.value("url").isString(), "url field not found");
            const QString url = element.value("url").toString();
            CHECK(element.contains("name") && element.value("name").isString(), "name field not found");
            const QString name = element.value("name").toString();
            CHECK(element.contains("isExternal") && element.value("isExternal").isBool(), "isExternal field not found");
            const bool isExternal = element.value("isExternal").toBool();
            bool isDefault = false;
            if (element.contains("isDefault") && element.value("isDefault").isBool()) {
                isDefault = element.value("isDefault").toBool();
            }

            mappingsPages[name] = PageInfo(url, isExternal, isDefault);
        }
    } catch (const Exception &e) {
        LOG << "Error: " + e;
    } catch (...) {
        LOG << "Unknown error";
    }
}

void MainWindow::getIpsServers(QString requestId, QString type, int length, int count) {
    const QString JS_NAME_RESULT = "getIpsServersJs";

    LOG << "get ips servers " << requestId;

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &type, length, count]() {
        const std::vector<QString> result = nsLookup.getRandom(type, length, count);

        QString resultStr = "[";
        bool isFirst = true;
        for (const QString &r: result) {
            if (!isFirst) {
                resultStr += ", ";
            }
            isFirst = false;
            resultStr += "\\\"" + r + "\\\"";
        }
        resultStr += "]";

        const QString jScript = JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + resultStr + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        ui->webView->page()->runJavaScript(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }

    LOG << "Create wallet ok " << requestId;
}

void MainWindow::showExpanded() {
    show();
}
