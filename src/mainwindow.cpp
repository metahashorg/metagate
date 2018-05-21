#include "mainwindow.h"

#include <iostream>
#include <fstream>

#include <QWebEnginePage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QStandardPaths>
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
const static QString WALLET_PATH_MTH = "mth/";

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

MainWindow::MainWindow(ServerName &serverName, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , serverName(serverName)
{
    ui->setupUi(this);

    configureMenu();

    currentBeginPath = Uploader::getPagesPath();

    hardReloadPage();

    channel = std::make_unique<QWebChannel>(ui->webView->page());
    ui->webView->page()->setWebChannel(channel.get());
    channel->registerObject(QString("mainWindow"), this);

    walletDefaultPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath(WALLET_PATH_DEFAULT);
    LOG << "Wallets default path " << walletPath.toStdString() << std::endl;

    ui->webView->setContextMenuPolicy(Qt::CustomContextMenu);
    CHECK(connect(ui->webView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))), "not connect");

    setPaths(walletDefaultPath, "");
    walletPathMth = QDir(walletPathMth).filePath("../");
}

void MainWindow::listSettingsClicked(const QModelIndex &index) {
    const QString name = index.data(INDEX_DESCRIPTION_LIST_ITEM).toString();
    if (name == "settings") {
        ui->webView->page()->runJavaScript("qToolBarClick(\"settings\")");
    } else {
        ui->webView->page()->runJavaScript("qToolBarClick(\"" + name + "\")");
    }
}

void MainWindow::listUserClicked(const QModelIndex &index) {
    const QString name = index.data(INDEX_DESCRIPTION_LIST_ITEM).toString();
    ui->webView->page()->runJavaScript("qToolBarClick(\"" + name + "\")");
    logoutMenu->setVisible(false);
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
    configureMenuButton(ui->settingsButton, ":/resources/svg/menu.svg", ":/resources/svg/menu_white.svg");

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

    CHECK(connect(ui->backButton, SIGNAL(pressed()), ui->webView, SLOT(back())), "not connect");
    ui->backButton->setEnabled(false);
    QAction *action = ui->webView->page()->action(QWebEnginePage::Back);
    CHECK(connect(action, &QAction::changed, [action, btn=ui->backButton]{
        btn->setEnabled(action->isEnabled());
    }), "Not connect");

    CHECK(connect(ui->forwardButton, SIGNAL(pressed()), ui->webView, SLOT(forward())), "not connect");
    ui->forwardButton->setEnabled(false);
    QAction *action2 = ui->webView->page()->action(QWebEnginePage::Forward);
    CHECK(connect(action2, &QAction::changed, [action2, btn=ui->forwardButton]{
        btn->setEnabled(action2->isEnabled());
    }), "Not connect");

    CHECK(connect(ui->refreshButton, SIGNAL(pressed()), ui->webView, SLOT(reload())), "not connect");

    auto addItemToSettings = [this](QMenu *menu, const QString &iconPath, const QString &title, const QString &name) {
        menu->addAction(QIcon(iconPath), title, [this, name](){
            ui->webView->page()->runJavaScript("qToolBarClick(\"" + name + "\")");
        });
    };

    auto addItemWithoutIcon = [this](QMenu *menu, const QString &title, const QString &name) {
        menu->addAction(title, [this, name](){
            ui->webView->page()->runJavaScript("qToolBarClick(\"" + name + "\")");
        });
    };

    addItemToSettings(settingsList, ":/resources/svg/settigs.svg", "Settings", "settings");
    addItemToSettings(settingsList, ":/resources/svg/English.svg", "English", "en");
    /*addItemToSettings(settingsList, ":/resources/svg/russian.svg", "Русский", "ru");
    addItemToSettings(settingsList, ":/resources/svg/Espanol.svg", "Español", "es");
    addItemToSettings(settingsList, ":/resources/svg/Portugales.svg", "Português", "po");
    addItemToSettings(settingsList, ":/resources/svg/Turce.svg", "Türce", "tu");
    addItemToSettings(settingsList, ":/resources/svg/Bahasa Melayu.svg", "Bahasa Melayu", "bm");
    addItemToSettings(settingsList, ":/resources/svg/3.svg", "中文", "k1");
    addItemToSettings(settingsList, ":/resources/svg/4.svg", "日本語", "k2");
    addItemToSettings(settingsList, ":/resources/svg/2.svg", "한국어", "k3");
    addItemToSettings(settingsList, ":/resources/svg/1.svg", "ﺔﻴﺑﺮﻌﻟا", "k4");*/
    ui->settingsButton->setMenu(settingsList);

    addItemWithoutIcon(loginMenu, "login", "login");
    addItemWithoutIcon(logoutMenu, "logout", "logout");

    CHECK(connect(ui->userButton, &QAbstractButton::pressed, [this, btn=ui->userButton]{
        if (!btn->text().isEmpty()) {
            btn->setMenu(logoutMenu);
        } else {
            btn->setMenu(loginMenu);
        }
        btn->showMenu();
    }), "not connect");

    CHECK(connect(ui->buyButton, &QAbstractButton::pressed, [this]{
        ui->webView->page()->runJavaScript("qToolBarClick(\"buymhc\")");
    }), "Not connect");

    CHECK(connect(ui->metaWalletButton, &QAbstractButton::pressed, [this]{
        ui->webView->page()->runJavaScript("qToolBarClick(\"wallet\")");
    }), "Not connect");

    CHECK(connect(ui->metaAppsButton, &QAbstractButton::pressed, [this]{
        ui->webView->page()->runJavaScript("qToolBarClick(\"apps\")");
    }), "Not connect");
}

void MainWindow::registerCommandLine() {
    CHECK(connect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(lineEditReturnPressed(const QString&))), "not connect");
}

void MainWindow::unregisterCommandLine() {
    CHECK(disconnect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(lineEditReturnPressed(const QString&))), "not connect");
}

void MainWindow::lineEditReturnPressed(const QString &text) {
    const QString METAHASH_URL = "metahash://";
    const QString METAGATE_URL = "/MetaGate/";

    LOG << "command line " << text.toStdString() << std::endl;
    if (ui->commandLine->count() <= 1) {
        //return;
    }

    const QString url = text;
    if (url.startsWith(METAGATE_URL)) {
        ui->webView->page()->runJavaScript("window.qToolBarAddressChange(\"" + url + "\");");
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

        LOG << "switch to url " << uri.toStdString() << std::endl;
        LOG << "other " << other.toStdString() << std::endl;
        QWebEngineHttpRequest req("http://127.0.0.1" + other);
        req.setHeader("host", uri.toUtf8());
        hardReloadPage2(req);
    } else {
        hardReloadPage2(url);
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
        LOG << "Error " << e << std::endl;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what() << std::endl;
    } catch (...) {
        LOG << "Unknown error" << std::endl;
    }
}

void MainWindow::updateAppEvent(const QString appVersion, const QString reference, const QString message) {
    try {
        const QString currentVersion = VERSION_STRING;
        const QString jsScript = "window.onQtAppUpdate  && window.onQtAppUpdate(\"" + appVersion + "\", \"" + reference + "\", \"" + currentVersion + "\", \"" + message + "\");";
        LOG << "Update script " << jsScript.toStdString() << std::endl;
        ui->webView->page()->runJavaScript(jsScript);
    } catch (const Exception &e) {
        LOG << "Error " << e << std::endl;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what() << std::endl;
    } catch (...) {
        LOG << "Unknown error" << std::endl;
    }
}

void MainWindow::softReloadPage() {
    LOG << "updateReady()" << std::endl;
    ui->webView->page()->runJavaScript("updateReady();");
}

void MainWindow::softReloadApp() {
    QApplication::exit(RESTART_BROWSER);
}

void MainWindow::hardReloadPage2(const QString &page) {
    ui->webView->page()->profile()->setRequestInterceptor(nullptr);
    ui->webView->load(page);
    LOG << "Reload ok" << std::endl;
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
    LOG << "Reload ok" << std::endl;
}

void MainWindow::hardReloadPage() {
    const auto &lastVersionPair = Uploader::getLastVersion(currentBeginPath);
    const auto &folderName = lastVersionPair.first;
    const auto &lastVersion = lastVersionPair.second;

    LOG << "Reload. Last version " << lastVersion.toStdString() << std::endl;
    ui->webView->page()->profile()->setRequestInterceptor(nullptr);
    hardReloadPage2("file:///" + QDir(QDir(QDir(currentBeginPath).filePath(folderName)).filePath(lastVersion)).filePath("login.html"));
}

template<class Function>
static TypedException apiVrapper(const Function &func) {
    try {
        func();
        return TypedException(TypeErrors::NOT_ERROR, "");
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description << std::endl;
        return e;
    } catch (const Exception &e) {
        LOG << "Error " << e << std::endl;
        return TypedException(TypeErrors::OTHER_ERROR, e);
    } catch (const std::exception &e) {
        LOG << "Error " << e.what() << std::endl;
        return TypedException(TypeErrors::OTHER_ERROR, e.what());
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return TypedException(TypeErrors::OTHER_ERROR, "Unknown error");
    }
}

////////////////
/// METAHASH ///
////////////////

void MainWindow::createWallet(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletResultJs";

    LOG << "Create wallet " << requestId.toStdString() << std::endl;

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &password]() {
        std::string publicKey;
        std::string addr;
        const std::string exampleMessage = "Example message " + std::to_string(rand());
        std::string signature;

        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createWallet(walletPathMth, password.toStdString(), publicKey, addr);

        publicKey.clear();
        Wallet wallet(walletPathMth, addr, password.toStdString());
        signature = wallet.sign(exampleMessage, publicKey);

        const QString jScript = JS_NAME_RESULT + "(" +
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
        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
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

    LOG << "Create wallet ok " << requestId.toStdString() << std::endl;
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

QString MainWindow::getAllWalletsAndPathsJson() {
    try {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPathMth);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get mth wallets json " << jsonStr.toStdString() << std::endl;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e << std::endl;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return "Unknown error";
    }
}

QString MainWindow::getAllWalletsJson() {
    try {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPathMth);
        const QString jsonStr = makeJsonWallets(result);
        LOG << "get mth wallets json " << jsonStr.toStdString() << std::endl;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e << std::endl;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return "Unknown error";
    }
}

void MainWindow::signMessage(QString requestId, QString keyName, QString text, QString password) {
    const QString JS_NAME_RESULT = "signMessageResultJs";

    LOG << requestId.toStdString() << std::endl;
    LOG << keyName.toStdString() << std::endl;
    LOG << text.toStdString() << std::endl;

    const std::string textStr = text.toStdString();

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &keyName, &textStr, &password]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPathMth, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        const std::string signature = wallet.sign(textStr, publicKey);

        ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(signature) + "\", " +
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

    LOG << "Create wallet eth " << requestId.toStdString() << std::endl;

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

    LOG << "Create eth wallet ok " << requestId.toStdString() << std::endl;
}

void MainWindow::signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data) {
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message eth" << std::endl;

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
        LOG << "get eth wallets json " << jsonStr.toStdString() << std::endl;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e << std::endl;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return "Unknown error";
    }
}

QString MainWindow::getAllEthWalletsAndPathsJson() {
    try {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPathEth);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get eth wallets json " << jsonStr.toStdString() << std::endl;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e << std::endl;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return "Unknown error";
    }
}

///////////////
/// BITCOIN ///
///////////////

void MainWindow::createWalletBtcPswd(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId.toStdString() << std::endl;

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

    LOG << "Create btc wallet ok " << requestId.toStdString() << std::endl;
}

void MainWindow::createWalletBtc(QString requestId) {
    createWalletBtcPswd(requestId, "");
}

void MainWindow::signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
    const QString JS_NAME_RESULT = "signMessageBtcResultJs";

    LOG << "Sign message btc" << std::endl;

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
        LOG << "get btc wallets json " << jsonStr.toStdString() << std::endl;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e << std::endl;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return "Unknown error";
    }
}

QString MainWindow::getAllBtcWalletsAndPathsJson() {
    try {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPathBtc);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get btc wallets json " << jsonStr.toStdString() << std::endl;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e << std::endl;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error" << std::endl;
        return "Unknown error";
    }
}

//////////////
/// COMMON ///
//////////////

void MainWindow::updateAndReloadApplication() {
    const QString JS_NAME_RESULT = "reloadApplicationJs";

    LOG << "Reload application " << std::endl;

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
    LOG << "Open another url " << url.toStdString() << std::endl;
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::getWalletFolders() {
    LOG << "getWalletFolders " << std::endl;
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
    LOG << "Migrate keys to path " << newPath.toStdString() << std::endl;

    const QString prevPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath(WALLET_PREV_PATH);

    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_ETH), QDir(newPath).filePath(WALLET_PATH_ETH), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_BTC), QDir(newPath).filePath(WALLET_PATH_BTC), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_MTH), QDir(newPath).filePath(WALLET_PATH_MTH), false);
    copyRecursively(prevPath, QDir(newPath).filePath(WALLET_PATH_MTH), false);

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
        LOG << "Wallets path " << walletPath.toStdString() << std::endl;

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
    LOG << "choised dir " << dir.toStdString() << std::endl;
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

    const QString uid = QString::fromStdString("\"" + ::getMachineUid() + "\"");
    ui->webView->page()->runJavaScript(JS_NAME_RESULT + "(" +
        uid + "" +
        ");"
    );
}

void MainWindow::setHasNativeToolbarVariable() {
    ui->webView->page()->runJavaScript("window.hasNativeToolbar = true;");
}

void MainWindow::setCommandLineText(const QString &text) {
    LOG << "scl " << text.toStdString() << std::endl;

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

    registerCommandLine();
}

void MainWindow::openWalletPathInStandartExplorer() {
    QDesktopServices::openUrl("file:///" + walletPath);
}

void MainWindow::showExpanded() {
    show();
}
