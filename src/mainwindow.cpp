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
#include "algorithms.h"

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
    QJsonObject allJson;
    allJson.insert("app", "MetaSearch");
    QJsonObject data;
    data.insert("machine_uid", hardwareId);
    data.insert("user_id", userId);
    data.insert("focus_release_count", (int)focusCount);
    data.insert("text", QString(line.toUtf8().toHex()));
    data.insert("is_enter_pressed", isEnter);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

static QString makeMessageApplicationForWss(const QString &hardwareId, const QString &userId, const QString &applicationVersion, const QString &interfaceVersion) {
    QJsonObject allJson;
    allJson.insert("app", "MetaGate");
    QJsonObject data;
    data.insert("machine_uid", hardwareId);
    data.insert("user_id", userId);
    data.insert("application_ver", applicationVersion);
    data.insert("interface_ver", interfaceVersion);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

MainWindow::MainWindow(WebSocketClient &webSocketClient, JavascriptWrapper &jsWrapper, const QString &applicationVersion, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , webSocketClient(webSocketClient)
    , jsWrapper(jsWrapper)
    , applicationVersion(applicationVersion)
{
    ui->setupUi(this);

    hardwareId = QString::fromStdString(::getMachineUid());

    configureMenu();

    currentBeginPath = Uploader::getPagesPath();
    const auto &lastVersionPair = Uploader::getLastVersion(currentBeginPath);
    folderName = lastVersionPair.first;
    lastVersion = lastVersionPair.second;

    hardReloadPage("login.html");
    setCommandLineText2("app://Login", true, true);

    jsWrapper.setWidget(this);

    client.setParent(this);
    CHECK(connect(&client, SIGNAL(callbackCall(ReturnCallback)), this, SLOT(callbackCall(ReturnCallback))), "not connect");

    CHECK(connect(&jsWrapper, SIGNAL(jsRunSig(QString)), this, SLOT(onJsRun(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setHasNativeToolbarVariableSig()), this, SLOT(onSetHasNativeToolbarVariable())), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setCommandLineTextSig(QString)), this, SLOT(onSetCommandLineText(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setUserNameSig(QString)), this, SLOT(onSetUserName(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(setMappingsSig(QString)), this, SLOT(onSetMappings(QString))), "not connect");
    CHECK(connect(&jsWrapper, SIGNAL(lineEditReturnPressedSig(QString)), this, SLOT(enterCommandAndAddToHistory(QString))), "not connect");

    channel = std::make_unique<QWebChannel>(ui->webView->page());
    ui->webView->page()->setWebChannel(channel.get());
    channel->registerObject(QString("mainWindow"), &jsWrapper);

    ui->webView->setContextMenuPolicy(Qt::CustomContextMenu);
    CHECK(connect(ui->webView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))), "not connect");

    CHECK(connect(ui->webView->page(), &QWebEnginePage::loadFinished, this, &MainWindow::browserLoadFinished), "not connect");

    /*CHECK(connect(ui->webView->page(), &QWebEnginePage::urlChanged, [this](const QUrl &url) {
        if (url.toString().startsWith(METAHASH_URL)) {
            LOG << "Url changed!!! " << url.toString();
            lineEditReturnPressed(url.toString());
        }
        LOG << "Url not changed!!! " << url.toString();
    }), "not connect");*/

    qtimer.setInterval(hours(1).count());
    qtimer.setSingleShot(false);
    CHECK(connect(&qtimer, SIGNAL(timeout()), this, SLOT(updateMhsReferences())), "not connect");

    emit updateMhsReferences();

    sendAppInfoToWss(true);
}

void MainWindow::sendAppInfoToWss(bool force) {
    const QString newUserName = ui->userButton->text();
    if (force || newUserName != sendedUserName) {
        emit webSocketClient.sendMessage(makeMessageApplicationForWss(hardwareId, newUserName, applicationVersion, lastVersion));
        sendedUserName = newUserName;
    }
}

void MainWindow::updateMhsReferences() {
    client.sendMessageGet(QUrl("http://dns.metahash.io/"), [this](const std::string &response) {
        onSetMappingsMh(QString::fromStdString(response));
    });
}

void MainWindow::callbackCall(ReturnCallback callback) {
    try {
        callback();
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
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
            "QAbstractButton:hover { background-color: #4F4F4F; color: white; border-radius: 5px;} "
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
    ui->commandLine->setStyleSheet(
        "QComboBox {color: rgb(99, 99, 99); border-radius: 14px; padding-left: 14px; padding-right: 14px; } "
        "QComboBox::drop-down {padding-top: 10px; padding-right: 10px; width: 10px; height: 10px; image: url(:/resources/svg/arrow.svg);}"
    );
    ui->commandLine->setAttribute(Qt::WA_MacShowFocusRect, 0);

    registerCommandLine();

    CHECK(connect(ui->backButton, &QToolButton::pressed, [this] {
        historyPos--;
        enterCommandAndAddToHistory(history.at(historyPos - 1), false, false);
        ui->backButton->setEnabled(historyPos > 1);
        ui->forwardButton->setEnabled(historyPos < history.size());
    }), "not connect");
    ui->backButton->setEnabled(false);

    CHECK(connect(ui->forwardButton, &QToolButton::pressed, [this]{
        historyPos++;
        enterCommandAndAddToHistory(history.at(historyPos - 1), false, false);
        ui->backButton->setEnabled(historyPos > 1);
        ui->forwardButton->setEnabled(historyPos < history.size());
    }), "not connect");
    ui->forwardButton->setEnabled(false);

    CHECK(connect(ui->refreshButton, SIGNAL(pressed()), ui->webView, SLOT(reload())), "not connect");

    CHECK(connect(ui->userButton, &QAbstractButton::pressed, [this]{
        enterCommandAndAddToHistory("Settings");
    }), "not connect");

    CHECK(connect(ui->buyButton, &QAbstractButton::pressed, [this]{
        enterCommandAndAddToHistory("BuyMHC");
    }), "Not connect");

    CHECK(connect(ui->metaWalletButton, &QAbstractButton::pressed, [this]{
        enterCommandAndAddToHistory("Wallet");
    }), "Not connect");

    CHECK(connect(ui->metaAppsButton, &QAbstractButton::pressed, [this]{
        enterCommandAndAddToHistory("MetaApps");
    }), "Not connect");

    CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::editingFinished, [this]{
        countFocusLineEditChanged++;
    }), "Not connect");

    CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::textChanged, [this](const QString &text){
        emit webSocketClient.sendMessage(makeMessageForWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, text, false));
    }), "Not connect");
    CHECK(connect(ui->commandLine->lineEdit(), &QLineEdit::returnPressed, [this]{
        emit webSocketClient.sendMessage(makeMessageForWss(hardwareId, ui->userButton->text(), countFocusLineEditChanged, ui->commandLine->lineEdit()->text(), true));
        ui->commandLine->lineEdit()->setText(currentTextCommandLine);
    }), "Not connect");
}

void MainWindow::registerCommandLine() {
    CHECK(connect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(enterCommandAndAddToHistoryNoDuplicate(const QString&))), "not connect");
}

void MainWindow::unregisterCommandLine() {
    CHECK(disconnect(ui->commandLine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(enterCommandAndAddToHistoryNoDuplicate(const QString&))), "not connect");
}

void MainWindow::enterCommandAndAddToHistory(const QString &text) {
    enterCommandAndAddToHistory(text, true, false);
}

void MainWindow::enterCommandAndAddToHistoryNoDuplicate(const QString &text) {
    enterCommandAndAddToHistory(text, true, true);
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

bool MainWindow::compareTwoPaths(const QString &path1, const QString &path2) {
    PathParsed p1(path1);
    PathParsed p2(path2);

    if (p1.type == p2.type || p1.type == PathParsed::Type::NONE || p2.type == PathParsed::Type::NONE) {
        const QString lowerPath1 = p1.path.toLower();
        const QString lowerPath2 = p2.path.toLower();
        const auto found1 = mappingsPages.find(lowerPath1);
        const auto found2 = mappingsPages.find(lowerPath2);
        if (found1 == mappingsPages.end() || found2 == mappingsPages.end()) {
            return lowerPath1 == lowerPath2;
        } else {
            return found1->second.page == found2->second.page;
        }
    } else {
        return false;
    }
}

void MainWindow::enterCommandAndAddToHistory(const QString &text1, bool isAddToHistory, bool isNoEnterDuplicate) {
    LOG << "command line " << text1;

    const QString HTTP_1_PREFIX = "http://";
    const QString HTTP_2_PREFIX = "https://";

    QString text = text1;
    if (text.endsWith('/')) {
        text = text.left(text.size() - 1);
    }

    if (isNoEnterDuplicate && text == currentTextCommandLine) {
        return;
    }

    auto runSearch = [&, this](const QString &url) {
        QTextDocument td;
        td.setHtml(url);
        const QString plained = td.toPlainText();
        QString link = "";
        QString printedName;
        for (const auto pageInfoIt: mappingsPages) {
            if (pageInfoIt.second.isDefault) {
                link = pageInfoIt.second.page;
                printedName = pageInfoIt.second.printedName;
            }
        }
        if (link.isNull() || link.isEmpty()) {
            LOG << "Error. Not found search url in mappings.";
            return;
        }
        link += plained;
        LOG << "Founded page " << link;
        setCommandLineText2(printedName + ":" + url, isAddToHistory, true);
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

    PageInfo pageInfo;
    const auto found = mappingsPages.find(text.toLower());
    if (found != mappingsPages.end()) {
        pageInfo = found->second;
    } else if (!text.startsWith(METAHASH_URL) && !text.startsWith(APP_URL)) {
        const QString appUrl = APP_URL + text.toLower();
        const auto found2 = mappingsPages.find(appUrl);
        if (found2 != mappingsPages.end()) {
            pageInfo = found2->second;
        } else if (isFullUrl(text)) {
            pageInfo.page = METAHASH_URL + text;
        }
    } else if (text.startsWith(METAHASH_URL)){
        pageInfo.page = text;
    } else {
        CHECK(text.startsWith(APP_URL), "Incorrect text: " + text.toStdString());
        text = text.mid(APP_URL.size());
    }
    const QString &reference = pageInfo.page;

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
        QString ip;
        if (!pageInfo.ips.empty()) {
            ip = ::getRandom(pageInfo.ips);
        } else {
            CHECK(!defaultMhIps.empty(), "defaults mh ips empty");
            ip = ::getRandom(defaultMhIps);
        }
        LOG << "ip " << ip;
        QWebEngineHttpRequest req(ip + other);
        req.setHeader("host", uri.toUtf8());
        QString clText;
        if (pageInfo.printedName.isNull() || pageInfo.printedName.isEmpty()) {
            clText = reference;
        } else {
            clText = pageInfo.printedName;
        }
        setCommandLineText2(clText, isAddToHistory, true);
        hardReloadPage2(req);
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
            setCommandLineText2(clText, isAddToHistory, true);
            hardReloadPage2(reference);
        } else {
            setCommandLineText2(clText, isAddToHistory, true);
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
    LOG << "Reload. Last version " << lastVersion;
    ui->webView->page()->profile()->setRequestInterceptor(nullptr);
    hardReloadPage2("file:///" + QDir(QDir(QDir(currentBeginPath).filePath(folderName)).filePath(lastVersion)).filePath(pageName));
}

void MainWindow::setCommandLineText2(const QString &text, bool isAddToHistory, bool isReplace) {
    LOG << "scl " << text;

    unregisterCommandLine();
    const QString currText = ui->commandLine->currentText();
    bool isSetText = true;
    if (!currText.isEmpty()) {
        if (ui->commandLine->count() >= 1) {
            if (!compareTwoPaths(ui->commandLine->itemText(ui->commandLine->count() - 1), currText)) {
                ui->commandLine->addItem(currText);
            }
        } else {
            ui->commandLine->addItem(currText);
        }
    }
    if (compareTwoPaths(currText, text)) {
        isSetText = isReplace;
    }
    if (isSetText) {
        ui->commandLine->setCurrentText(text);
    }

    currentTextCommandLine = text;

    if (isAddToHistory && isSetText) {
        if (historyPos == 0 || !compareTwoPaths(history[historyPos - 1], text)) {
            history.insert(history.begin() + historyPos, text);
            historyPos++;

            ui->backButton->setEnabled(history.size() > 1);
        } else if (compareTwoPaths(history[historyPos - 1], text)) {
            history.at(historyPos - 1) = text;
        }
    }

    registerCommandLine();
}

void MainWindow::browserLoadFinished(bool result) {
    if (!result) {
        return;
    }
    const QString url = ui->webView->url().toString();
    auto found = urlToName.find(url);
    if (found == urlToName.end()) {
        const int find = url.lastIndexOf('/');
        if (find != -1) {
            const QString lastUrl = url.mid(find + 1);
            found = urlToName.find(lastUrl);
        }
    }
    if (found != urlToName.end()) {
        LOG << "Set address after load " << found->second;
        setCommandLineText2(found->second, true, false);
    }
}

void MainWindow::onSetCommandLineText(QString text) {
    setCommandLineText2(text, true, true);
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

    sendAppInfoToWss(false);
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
            bool isPreferred = false;
            if (element.contains("isPreferred") && element.value("isPreferred").isBool()) {
                isPreferred = element.value("isPreferred").toBool();
            }
            bool isLocalFile = true;
            if (element.contains("isLocalFile") && element.value("isLocalFile").isBool()) {
                isLocalFile = element.value("isLocalFile").toBool();
            }

            PageInfo pageInfo(url, isExternal, isDefault, isLocalFile);
            if (!name.startsWith(APP_URL) && !name.startsWith(METAHASH_URL)) {
                pageInfo.printedName = APP_URL + name;
            } else {
                pageInfo.printedName = name;
            }
            mappingsPages[name.toLower()] = pageInfo;

            auto foundUrlToName = urlToName.find(url);
            if (foundUrlToName == urlToName.end()) {
                urlToName[url] = pageInfo.printedName;
            } else if (isPreferred) {
                urlToName[url] = pageInfo.printedName;
            }
        }
    } catch (const Exception &e) {
        LOG << "Error: " + e;
    } catch (...) {
        LOG << "Unknown error";
    }
}

static QString ipToHttp(const QString &ip) {
    const static QString HTTP = "http://";
    if (ip.startsWith(HTTP)) {
        return ip;
    } else {
        return HTTP + ip;
    }
}

void MainWindow::onSetMappingsMh(QString mapping) {
    LOG << "Set mappings mh " << mapping;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(mapping.toUtf8(), &parseError);
    CHECK(parseError.error == QJsonParseError::NoError, "Json parse error: " + parseError.errorString().toStdString());
    const QJsonObject root = document.object();
    CHECK(root.contains("ext") && root.value("ext").isArray(), "ext field not found ");
    const QJsonArray &routes = root.value("ext").toArray();
    for (const QJsonValue &value: routes) {
        CHECK(value.isObject(), "value array incorrect type");
        const QJsonObject &element = value.toObject();
        CHECK(element.contains("type") && element.value("type").isString(), "type field not found");
        const QString type = element.value("type").toString();
        const bool isDefault = type == "defaultGateway";
        if (!isDefault) {
            CHECK(element.contains("url") && element.value("url").isString(), "url field not found");
            const QString url = element.value("url").toString();
            CHECK(element.contains("name") && element.value("name").isString(), "name field not found");
            const QString name = element.value("name").toString();
            CHECK(element.contains("isExternal") && element.value("isExternal").isBool(), "isExternal field not found");
            const bool isExternal = element.value("isExternal").toBool();
            std::vector<QString> ips;
            if (element.contains("ip") && element.value("ip").isArray()) {
                for (const QJsonValue &ip: element.value("ip").toArray()) {
                    CHECK(ip.isString(), "ips array incorrect type");
                    ips.emplace_back(ipToHttp(ip.toString()));
                }
            }

            PageInfo page(url, isExternal, isDefault, false);
            page.printedName = METAHASH_URL + name;
            page.ips = ips;

            auto addToMap = [this](auto &map, const QString &key, const PageInfo &page) {
                QString lowerKey = key.toLower();
                if (lowerKey.endsWith('/')) {
                    lowerKey = lowerKey.left(lowerKey.size() - 1);
                }
                auto found = map.find(lowerKey);
                if (found == map.end() || found->second.page.startsWith(METAHASH_URL)) { // Данные из javascript имеют приоритет
                    map[lowerKey] = page;
                }
            };

            addToMap(mappingsPages, name, page); // TODO заменить на shared_ptr или придумать другую схему
            addToMap(mappingsPages, url, page);
            addToMap(mappingsPages, page.printedName, page);

            if (element.contains("aliases") && element.value("aliases").isArray()) {
                for (const QJsonValue &alias: element.value("aliases").toArray()) {
                    CHECK(alias.isString(), "aliases array incorrect type");
                    addToMap(mappingsPages, alias.toString(), page);
                }
            }
        } else {
            if (element.contains("ip") && element.value("ip").isArray()) {
                std::vector<QString> ips;
                for (const QJsonValue &ip: element.value("ip").toArray()) {
                    CHECK(ip.isString(), "ips array incorrect type");
                    ips.emplace_back(ipToHttp(ip.toString()));
                }
                defaultMhIps = ips;
            }
        }
    }
}

void MainWindow::onJsRun(QString jsString) {
    ui->webView->page()->runJavaScript(jsString);
}

void MainWindow::showExpanded() {
    show();
}
