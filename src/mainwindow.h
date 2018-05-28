#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <map>

#include <QMainWindow>
#include <QWebChannel>
#include <QStandardItem>
#include <QListView>

#include "ui_mainwindow.h"

#include "WindowEvents.h"

class WebSocketClient;
class JavascriptWrapper;

namespace Ui {
    class MainWindow;
}

struct PageInfo {
    QString page;
    bool isExternal;
    bool isDefault = false;

    PageInfo() = default;

    PageInfo(const QString &page, bool isExternal, bool isDefault)
        : page(page)
        , isExternal(isExternal)
        , isDefault(isDefault)
    {}
};

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

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    explicit MainWindow(WebSocketClient &webSocketClient, JavascriptWrapper &jsWrapper, QWidget *parent = 0);

    void showExpanded();

    void softReloadPage();

    void hardReloadPage(const QString &pageName);

    void softReloadApp();

private:
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<QWebChannel> channel;

    void hardReloadPage2(const QString &page);

    void hardReloadPage2(const QWebEngineHttpRequest &page);

    void configureMenu();

    void registerCommandLine();

    void unregisterCommandLine();

public slots:

    void processEvent(WindowEvent event);

    void updateAppEvent(const QString appVersion, const QString reference, const QString message);

    void lineEditReturnPressed2(const QString &text1, bool isAddToHistory=true, bool isLineEditPressed=false);

    void lineEditReturnPressed(const QString &text);

    void lineEditReturnPressed3(const QString &text);

private:

    void setCommandLineText2(const QString &text, bool isAddToHistory=true);

    void setUserName(const QString &userName);

    void qtOpenInBrowser(QString url);

    void setHasNativeToolbarVariable();

    void setPagesMapping(QString mapping);

public slots:

    void ShowContextMenu(const QPoint &point);

    void contextMenuCut();

    void contextMenuCopy();

    void contextMenuPaste();

    void onJsRun(QString jsString);

    void onSetHasNativeToolbarVariable();

    void onSetCommandLineText(QString text);

    void onSetUserName(QString name);

    void onSetMappings(QString json);

signals:

    void newUpdate();

private:

    WebSocketClient &webSocketClient;

    JavascriptWrapper &jsWrapper;

    QString currentBeginPath;

    QString lastVersion;

    QString currentTextCommandLine;

    std::map<QString, PageInfo> mappingsPages;

    QString hardwareId;

    size_t countFocusLineEditChanged = 0;

    std::vector<QString> history;
    size_t historyPos = 0;
};

#endif // MAINWINDOW_H
