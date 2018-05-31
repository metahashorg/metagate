#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <map>

#include <QMainWindow>
#include <QWebChannel>
#include <QStandardItem>
#include <QListView>
#include <QTimer>

#include "client.h"

#include "ui_mainwindow.h"

#include "PagesMappings.h"

#include "WindowEvents.h"

class WebSocketClient;
class JavascriptWrapper;

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

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    explicit MainWindow(WebSocketClient &webSocketClient, JavascriptWrapper &jsWrapper, const QString &applicationVersion, QWidget *parent = 0);

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

    void sendAppInfoToWss(bool force);

public slots:

    void processEvent(WindowEvent event);

    void updateAppEvent(const QString appVersion, const QString reference, const QString message);

    void enterCommandAndAddToHistory(const QString &text1, bool isAddToHistory, bool isNoEnterDuplicate);

    void enterCommandAndAddToHistory(const QString &text);

    void enterCommandAndAddToHistoryNoDuplicate(const QString &text);

    void browserLoadFinished(bool result);

private:

    void setCommandLineText2(const QString &text, bool isAddToHistory, bool isReplace);

    void qtOpenInBrowser(QString url);

public slots:

    void callbackCall(ReturnCallback callback);

    void updateMhsReferences();

    void ShowContextMenu(const QPoint &point);

    void onJsRun(QString jsString);

    void onSetHasNativeToolbarVariable();

    void onSetCommandLineText(QString text);

    void onSetUserName(QString userName);

    void onSetMappings(QString mapping);

signals:

    void newUpdate();

private:

    WebSocketClient &webSocketClient;

    JavascriptWrapper &jsWrapper;

    QString sendedUserName;

    const QString applicationVersion;

    QString currentBeginPath;

    QString folderName;

    QString lastVersion;

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
};

#endif // MAINWINDOW_H
