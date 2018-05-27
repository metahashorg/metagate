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

class ServerName;
class NsLookup;
class WebSocketClient;

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

    explicit MainWindow(ServerName &serverName, NsLookup &nsLookup, WebSocketClient &webSocketClient, QWidget *parent = 0);

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

public slots:

    Q_INVOKABLE void createWallet(QString requestId, QString password);

    Q_INVOKABLE QString getAllWalletsJson();

    Q_INVOKABLE QString getAllWalletsAndPathsJson();

    Q_INVOKABLE void signMessage(QString requestId, QString keyName, QString text, QString password);

public slots:

    Q_INVOKABLE void createWalletMHC(QString requestId, QString password);

    Q_INVOKABLE QString getAllMHCWalletsJson();

    Q_INVOKABLE QString getAllMHCWalletsAndPathsJson();

    Q_INVOKABLE void signMessageMHC(QString requestId, QString keyName, QString text, QString password);

public slots:

    Q_INVOKABLE void createRsaKey(QString requestId, QString address, QString password);

    Q_INVOKABLE void decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex);

public slots:

    Q_INVOKABLE void createWalletEth(QString requestId, QString password);

    Q_INVOKABLE void signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data);

    //Q_INVOKABLE void signMessageTokensEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString contractAddress, QString to, QString value);

    Q_INVOKABLE QString getAllEthWalletsJson();

    Q_INVOKABLE QString getAllEthWalletsAndPathsJson();

public slots:

    Q_INVOKABLE void createWalletBtc(QString requestId);

    Q_INVOKABLE void createWalletBtcPswd(QString requestId, QString password);

    Q_INVOKABLE void signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);

    Q_INVOKABLE void signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);

    Q_INVOKABLE QString getAllBtcWalletsJson();

    Q_INVOKABLE QString getAllBtcWalletsAndPathsJson();

public slots:

    Q_INVOKABLE bool migrateKeysToPath(QString newPath);

    Q_INVOKABLE void updateAndReloadApplication();

    Q_INVOKABLE void qtOpenInBrowser(QString url);

    Q_INVOKABLE void getWalletFolders();

    Q_INVOKABLE void setPaths(QString newPatch, QString newUserName);

    Q_INVOKABLE QString openFolderDialog(QString beginPath, QString caption);

    Q_INVOKABLE void exitApplication();

    Q_INVOKABLE QString backupKeys(QString caption);

    Q_INVOKABLE QString restoreKeys(QString caption);

    Q_INVOKABLE void getMachineUid();

    Q_INVOKABLE void setUserName(const QString &userName);

    Q_INVOKABLE void setHasNativeToolbarVariable();

    Q_INVOKABLE void setCommandLineText(const QString &text);

    Q_INVOKABLE void openWalletPathInStandartExplorer();

    Q_INVOKABLE void setPagesMapping(QString mapping);

    Q_INVOKABLE void getIpsServers(QString requestId, QString type, int length, int count);

private:

    void setCommandLineText2(const QString &text, bool isAddToHistory=true);

    void createWalletMTHS(QString requestId, QString password, QString walletPath, QString jsNameResult);

    QString getAllMTHSWalletsJson(QString walletPath);

    QString getAllMTHSWalletsAndPathsJson(QString walletPath);

    void signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult);

public slots:

    void ShowContextMenu(const QPoint &point);

    void contextMenuCut();

    void contextMenuCopy();

    void contextMenuPaste();

signals:

    void newUpdate();

private:

    QMenu *settingsList;

    QMenu *logoutMenu;

    QMenu *loginMenu;

    ServerName &serverName;

    NsLookup &nsLookup;

    WebSocketClient &webSocketClient;

    QString savedToken;

    QString currentBeginPath;

    QString lastVersion;

    QString walletDefaultPath;

    QString walletPath;

    QString walletPathMth;

    QString walletPathOldTmh;

    QString walletPathTmh;

    QString walletPathEth;

    QString walletPathBtc;

    QString userName;

    QString currentTextCommandLine;

    std::map<QString, PageInfo> mappingsPages;

    QString hardwareId;

    size_t countFocusLineEditChanged = 0;

    std::vector<QString> history;
    size_t historyPos = 0;
};

#endif // MAINWINDOW_H
