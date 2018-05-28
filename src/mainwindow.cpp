#include "mainwindow.h"

#include <iostream>
#include <fstream>
#include <map>

#include <QWebEnginePage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QTextDocument>
#include <QLineEdit>
#include <QDesktopServices>
#include <QDir>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QKeyEvent>
#include <QMenu>
#include <QStandardItemModel>
#include <QFontDatabase>
#include <QWebEngineHistory>

#include "WebSocketClient.h"
#include "JavascriptWrapper.h"

#include "uploader.h"
#include "check.h"
#include "StopApplication.h"
#include "duration.h"
#include "Log.h"
#include "utils.h"

#include "machine_uid.h"

const static QString METAHASH_URL = "mh://";
const static QString APP_URL = "app://";

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

MainWindow::MainWindow(WebSocketClient &webSocketClient, JavascriptWrapper &jsWrapper, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , webSocketClient(webSocketClient)
    , jsWrapper(jsWrapper)
{
    ui->setupUi(this);

    hardwareId = QString::fromStdString(::getMachineUid());

    configureMenu();

    currentBeginPath = Uploader::getPagesPath();

    hardReloadPage("login.html");

    jsWrapper.setWidget(this);

    CHECK(connect(&jsWrapper, SIGNAL(jsRunSig(QString)), this, SLOT(onJsRun(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setHasNativeToolbarVariableSig()), this, SLOT(onSetHasNativeToolbarVariable())), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setCommandLineTextSig(QString)), this, SLOT(onSetCommandLineText(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setUserNameSig(QString)), this, SLOT(onSetUserName(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setMappingsSig(QString)), this, SLOT(onSetMappings(QString))), "not connect");

    channel = std::make_unique<QWebChannel>(ui->webView->page());
    ui->webView->page()->setWebChannel(channel.get());
    channel->registerObject(QString("mainWindow"), &jsWrapper);

    ui->webView->setContextMenuPolicy(Qt::CustomContextMenu);
    CHECK(connect(ui->webView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))), "not connect");
}

void MainWindow::configureMenu() {
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

    CHECK(connect(ui->userButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed("Settings");
    }), "not connect");

    CHECK(connect(ui->buyButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed("BuyMHC");
    }), "Not connect");

    CHECK(connect(ui->metaWalletButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed("Wallet");
    }), "Not connect");

    CHECK(connect(ui->metaAppsButton, &QAbstractButton::pressed, [this]{
        lineEditReturnPressed("Apps");
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
    CHECK(connect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(lineEditReturnPressed3(const QString&))), "not connect");
    /*CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::returnPressed, [this](){
        LOG << "Ya tuta " << ui->commandLine->lineEdit()->text().toStdString() << " " << currentTextCommandLine.toStdString() << std::endl;
        if (ui->commandLine->lineEdit()->text() == currentTextCommandLine) {
            LOG << "refresh" << std::endl;
            emit ui->refreshButton->pressed();
        }
    }), "not connect");*/
}

void MainWindow::unregisterCommandLine() {
    CHECK(disconnect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(lineEditReturnPressed3(const QString&))), "not connect");
    //CHECK(disconnect(ui->commandLine->lineEdit(), SIGNAL(returnPressed(const QString&))), "not connect");
}

void MainWindow::lineEditReturnPressed(const QString &text) {
    lineEditReturnPressed2(text);
}

void MainWindow::lineEditReturnPressed3(const QString &text) {
    lineEditReturnPressed2(text, true, true);
}

struct PathParsed {
    enum class Type {
        METAHASH, APP, NONE
    };

    QString path;

    Type type;

    PathParsed(const QString &url) {
        if (url.startsWith(METAHASH_URL)) {
            type = Type::METAHASH;
            path = url.mid(METAHASH_URL.size());
        } else if (url.startsWith(APP_URL)) {
            type = Type::APP;
            path = url.mid(APP_URL.size());
        } else {
            type = Type::NONE;
            path = url;
        }
    }
};

static bool compareTwoPaths(const QString &path1, const QString &path2) {
    PathParsed p1(path1);
    PathParsed p2(path2);

    if (p1.type == p2.type || p1.type == PathParsed::Type::NONE || p2.type == PathParsed::Type::NONE) {
        return p1.path == p2.path;
    } else {
        return false;
    }
}

void MainWindow::lineEditReturnPressed2(const QString &text1, bool isAddToHistory, bool isLineEditPressed) {
    LOG << "command line " << text1;

    QString text = text1;
    if (text.endsWith('/')) {
        text = text.left(text.size() - 1);
    }

    if (isLineEditPressed && text == currentTextCommandLine) {
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
            LOG << "Error. Not found search url in mappings.";
            return;
        }
        link += plained;
        LOG << "Founded page " << link;
        setCommandLineText2(url, isAddToHistory);
        //currentTextCommandLine = url;
        hardReloadPage(link);
    };

    auto isFullUrl = [](const QString &text) {
        if (text.size() != 52) {
            return false;
        }
        if (!isHex(text.toStdString())) {
            return false;
        }
        return true;
    };

    QString reference;
    bool isExternal = false;
    if (!text.startsWith(METAHASH_URL) && !text.startsWith(APP_URL)) {
        if (isFullUrl(text)) {
            const QString appUrl = APP_URL + text;
            const auto found = mappingsPages.find(appUrl);
            if (found != mappingsPages.end()) {
                reference = found->second.page;
                isExternal = found->second.isExternal;
            } else {
                reference = METAHASH_URL + text;
            }
        } else {
            const auto found = mappingsPages.find(text);
            if (found != mappingsPages.end()) {
                reference = found->second.page;
                isExternal = found->second.isExternal;
            } else {
                const auto found2 = mappingsPages.find(APP_URL + text);
                if (found2 != mappingsPages.end()) {
                    reference = found2->second.page;
                    isExternal = found2->second.isExternal;
                }
            }
        }
    } else if (text.startsWith(METAHASH_URL)){
        reference = text;
    } else {
        const auto found = mappingsPages.find(text);
        if (found != mappingsPages.end()) {
            reference = found->second.page;
            isExternal = found->second.isExternal;
        } else {
            text = text.mid(APP_URL.size());
        }
    }

    if (reference.isNull() || reference.isEmpty()) {
        runSearch(text);
    } else if (reference.startsWith(METAHASH_URL)) {
        QString uri = reference.mid(METAHASH_URL.size());
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
        if (isExternal) {
            qtOpenInBrowser(reference);
        } else {
            setCommandLineText2(text, isAddToHistory);
            hardReloadPage(reference);
        }
    }
}

void MainWindow::qtOpenInBrowser(QString url) {
    LOG << "Open another url " << url;
    QDesktopServices::openUrl(QUrl(url));
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
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier));
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_X, Qt::ControlModifier));
    }
}

void MainWindow::contextMenuCopy() {
    QWidget* focused = QApplication::focusWidget();
    if(focused != 0) {
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier));
    }
}

void MainWindow::contextMenuPaste() {
    QWidget* focused = QApplication::focusWidget();
    if(focused != 0) {
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
        QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier));
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

void MainWindow::setCommandLineText2(const QString &text, bool isAddToHistory) {
    LOG << "scl " << text;

    unregisterCommandLine();
    /*ui->commandLine->setItemText(ui->commandLine->currentIndex(), text);
    ui->commandLine->addItem(text);*/
    const QString currText = ui->commandLine->currentText();
    if (!currText.isEmpty()) {
        if (ui->commandLine->count() >= 1) {
            if (!compareTwoPaths(ui->commandLine->itemText(ui->commandLine->count() - 1), currText)) {
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
        if (historyPos == 0 || !compareTwoPaths(history[historyPos - 1], text)) {
            history.insert(history.begin() + historyPos, text);
            historyPos++;

            ui->backButton->setEnabled(history.size() > 1);
        }
    }

    registerCommandLine();
}

void MainWindow::onSetCommandLineText(QString text) {
    setCommandLineText2(text);
}

void MainWindow::onSetHasNativeToolbarVariable() {
    ui->webView->page()->runJavaScript("window.hasNativeToolbar = true;");
}

void MainWindow::onSetUserName(QString userName) {
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

void MainWindow::onSetMappings(QString mapping) {
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

void MainWindow::onJsRun(QString jsString) {
    ui->webView->page()->runJavaScript(jsString);
}

void MainWindow::showExpanded() {
    show();
}
