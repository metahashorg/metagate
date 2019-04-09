#ifndef JAVASCRIPTWRAPPER_H
#define JAVASCRIPTWRAPPER_H

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QDir>

#include "uploader.h"

#include "client.h"

#include "CallbackWrapper.h"

class NsLookup;
class WebSocketClient;
struct TypedException;
class MainWindow;

namespace transactions {
class Transactions;
}

namespace auth {
class Auth;
}

template<bool isLastArg, typename... Args>
struct JsFunc;

class JavascriptWrapper : public QObject
{
    Q_OBJECT
public:

    using ReturnCallback = std::function<void()>;

    using WalletsListCallback = CallbackWrapper<void(const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses)>;

public:

    enum class WalletType {
        Mth, Tmh, Btc, Eth
    };

public:
    explicit JavascriptWrapper(MainWindow &mainWindow, WebSocketClient &wssClient, NsLookup &nsLookup, transactions::Transactions &transactionsManager, auth::Auth &authManager, const QString &applicationVersion, QObject *parent = nullptr);

    void setWidget(QWidget *widget);

    void mvToThread(QThread *thread);

signals:

    void callbackCall(JavascriptWrapper::ReturnCallback callback);

    void jsRunSig(QString jsString);

    void setHasNativeToolbarVariableSig();

    void setCommandLineTextSig(QString text);

    void setMappingsSig(QString name);

    void lineEditReturnPressedSig(QString text);

    void sendCommandLineMessageToWssSig(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

    void mthWalletCreated(QString name);

public slots:

    void onLogined(bool isInit, const QString login);

signals:

    void getListWallets(const WalletType &type, const WalletsListCallback &callback);

private slots:

    void onGetListWallets(const WalletType &type, const WalletsListCallback &callback);

public slots:

    Q_INVOKABLE void createWallet(QString requestId, QString password);

    Q_INVOKABLE void checkWalletPassword(QString requestId, QString keyName, QString password);

    Q_INVOKABLE QString getAllWalletsJson();

    Q_INVOKABLE QString getAllWalletsAndPathsJson();

    Q_INVOKABLE void checkAddress(QString requestId, QString address);

    Q_INVOKABLE void signMessage(QString requestId, QString keyName, QString text, QString password);

    Q_INVOKABLE void signMessageV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex);

    Q_INVOKABLE void signMessageV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson);

    Q_INVOKABLE void signMessageDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);

    Q_INVOKABLE void signMessageUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);

    Q_INVOKABLE void getOnePrivateKey(QString requestId, QString keyName, bool isCompact);

    Q_INVOKABLE void saveRawPrivKey(QString requestId, QString rawPrivKey, QString password);

    Q_INVOKABLE void getRawPrivKey(QString requestId, QString address, QString password);

    Q_INVOKABLE void createV8Address(QString requestId, QString address, int nonce);

public slots:

    Q_INVOKABLE void createWalletMHC(QString requestId, QString password);

    Q_INVOKABLE void checkWalletPasswordMHC(QString requestId, QString keyName, QString password);

    Q_INVOKABLE QString getAllMHCWalletsJson();

    Q_INVOKABLE QString getAllMHCWalletsAndPathsJson();

    Q_INVOKABLE void signMessageMHC(QString requestId, QString keyName, QString text, QString password);

    Q_INVOKABLE void signMessageMHCV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex);

    Q_INVOKABLE void signMessageMHCV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson);

    Q_INVOKABLE void signMessageMHCDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);

    Q_INVOKABLE void signMessageMHCUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson);

    Q_INVOKABLE void getOnePrivateKeyMHC(QString requestId, QString keyName, bool isCompact);

    Q_INVOKABLE void saveRawPrivKeyMHC(QString requestId, QString rawPrivKey, QString password);

    Q_INVOKABLE void getRawPrivKeyMHC(QString requestId, QString address, QString password);

    Q_INVOKABLE void createV8AddressMHC(QString requestId, QString address, int nonce);

public slots:

    Q_INVOKABLE void createRsaKey(QString requestId, QString address, QString password);

    Q_INVOKABLE void getRsaPublicKey(QString requestId, QString address);

    Q_INVOKABLE void createRsaKeyMHC(QString requestId, QString address, QString password);

    Q_INVOKABLE void getRsaPublicKeyMHC(QString requestId, QString address);

    Q_INVOKABLE void encryptMessage(QString requestId, QString publicKey, QString message);

    Q_INVOKABLE void decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex);

    Q_INVOKABLE void copyRsaKey(QString requestId, QString address, QString pathPub, QString pathPriv);

    Q_INVOKABLE void copyRsaKeyMHC(QString requestId, QString address, QString pathPub, QString pathPriv);

    Q_INVOKABLE void copyRsaKeyToFolder(QString requestId, QString address, QString path);

    Q_INVOKABLE void copyRsaKeyToFolderMHC(QString requestId, QString address, QString path);

public slots:

    Q_INVOKABLE void createWalletEth(QString requestId, QString password);

    Q_INVOKABLE void signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data);

    Q_INVOKABLE void checkAddressEth(QString requestId, QString address);

    //Q_INVOKABLE void signMessageTokensEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString contractAddress, QString to, QString value);

    Q_INVOKABLE QString getAllEthWalletsJson();

    Q_INVOKABLE QString getAllEthWalletsAndPathsJson();

    Q_INVOKABLE void getOnePrivateKeyEth(QString requestId, QString keyName);

public slots:

    Q_INVOKABLE void createWalletBtc(QString requestId);

    Q_INVOKABLE void createWalletBtcPswd(QString requestId, QString password);

    Q_INVOKABLE void checkAddressBtc(QString requestId, QString address);

    Q_INVOKABLE void signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);

    Q_INVOKABLE void signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees);

    Q_INVOKABLE void signMessageBtcPswdUsedUtxos(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees, QString jsonUsedUtxos);

    Q_INVOKABLE QString getAllBtcWalletsJson();

    Q_INVOKABLE QString getAllBtcWalletsAndPathsJson();

    Q_INVOKABLE void getOnePrivateKeyBtc(QString requestId, QString keyName);

public slots:

    Q_INVOKABLE void savePrivateKeyAny(QString requestId, QString privateKey, QString password);

public slots:

    Q_INVOKABLE bool migrateKeysToPath(QString newPath);

    Q_INVOKABLE void updateAndReloadApplication();

    Q_INVOKABLE void qtOpenInBrowser(QString url);

    Q_INVOKABLE void getWalletFolders();

    Q_INVOKABLE void setPaths(QString newPatch, QString newUserName);

    Q_INVOKABLE QString openFolderDialog(QString beginPath, QString caption);

    Q_INVOKABLE void exitApplication();

    Q_INVOKABLE void restartBrowser();

    Q_INVOKABLE QString backupKeys(QString caption);

    Q_INVOKABLE QString restoreKeys(QString caption);

    Q_INVOKABLE void getMachineUid();

    Q_INVOKABLE void setUserName(const QString &userName);

    Q_INVOKABLE void setHasNativeToolbarVariable();

    Q_INVOKABLE void lineEditReturnPressed(QString text);

    Q_INVOKABLE void setCommandLineText(const QString &text);

    Q_INVOKABLE void openWalletPathInStandartExplorer();

    Q_INVOKABLE void setPagesMapping(QString mapping);

    Q_INVOKABLE void getIpsServers(QString requestId, QString type, int length, int count);

    Q_INVOKABLE void saveFileFromUrl(QString url, QString saveFileWindowCaption, QString fileName, bool openAfterSave);

    Q_INVOKABLE void printUrl(QString url, QString printWindowCaption, QString text);

    Q_INVOKABLE void chooseFileAndLoad(QString requestId, QString openFileWindowCaption, QString fileName);

    Q_INVOKABLE void getAppInfo(const QString requestId);

    Q_INVOKABLE void qrEncode(QString requestId, QString textHex);

    Q_INVOKABLE void qrDecode(QString requestId, QString pngBase64);

    Q_INVOKABLE void metaOnline();

    Q_INVOKABLE void getAppModules(const QString requestId);

    Q_INVOKABLE void clearNsLookup();

private slots:

    void onCallbackCall(ReturnCallback callback);

    void onDirChanged(const QString &dir);

    void onWssMessageReceived(QString message);

    void onSendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

private:

    void createRsaKeyMTHS(QString requestId, QString address, QString password, QString walletPath, QString jsNameResult);

    void getRsaPublicKeyMTHS(QString requestId, QString address, QString walletPath, QString jsNameResult);

    void copyRsaKeyMTHS(QString requestId, QString address, QString pathPub, QString pathPriv, QString walletPath, QString jsNameResult);

    void copyRsaKeyToFolderMTHS(QString requestId, QString address, QString path, QString walletPath, QString jsNameResult);

private:

    void setPathsImpl(QString newPatch, QString newUserName);

    void savePrivateKey(QString requestId, QString privateKey, QString password);

    void savePrivateKeyMHC(QString requestId, QString privateKey, QString password);

    void savePrivateKeyEth(QString requestId, QString privateKey, QString password);

    void savePrivateKeyBtc(QString requestId, QString privateKey, QString password);

    void saveRawPrivKeyMTHS(QString requestId, QString rawPrivKey, QString password, QString walletPath, QString jsNameResult);

    void getRawPrivKeyMTHS(QString requestId, QString address, QString password, QString walletPath, QString jsNameResult);

    void createWalletMTHS(QString requestId, QString password, QString walletPath, QString jsNameResult);

    void checkWalletPasswordMTHS(QString requestId, QString keyName, QString password, QString walletPath, QString jsNameResult);

    void getOnePrivateKeyMTHS(QString requestId, QString keyName, bool isCompact, QString walletPath, QString jsNameResult, bool isTmh);

    void savePrivateKeyMTHS(QString requestId, QString privateKey, QString password, QString walletPath, QString jsNameResult);

    QString getAllMTHSWalletsJson(QString walletPath, QString name);

    QString getAllMTHSWalletsAndPathsJson(QString walletPath, QString name);

    void signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult);

    void signMessageMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString walletPath, QString jsNameResult);

    void signMessageMTHSV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson, QString walletPath, QString jsNameResult);

    void signMessageDelegateMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, bool isDelegate, QString paramsJson, QString walletPath, QString jsNameResult);

    void signMessageMTHSWithTxManager(const QString &requestId, const QString &walletPath, const QString jsNameResult, const QString &nonce, const QString &keyName, const QString &password, const QString &paramsJson, const std::function<void(size_t nonce)> &signTransaction);

    void createV8AddressImpl(QString requestId, const QString jsNameResult, QString address, int nonce);

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args);

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

    void openFolderInStandartExplored(const QString &folder);

    void sendAppInfoToWss(QString userName, bool force);

    QByteArray getUtmData();

public:

    const QString walletDefaultPath;

    const static QString defaultUsername;

private:

    MainWindow &mainWindow;

    WebSocketClient &wssClient;

    NsLookup &nsLookup;

    transactions::Transactions &transactionsManager;

    const QString applicationVersion;

    QString sendedUserName;

    QString hardwareId;

    QString utmData;

    QString walletPath;

    QString walletPathMth;

    QString walletPathOldTmh;

    QString walletPathTmh;

    QString walletPathEth;

    QString walletPathBtc;

    QString userName;

    QWidget *widget_ = nullptr;

    SimpleClient client;

    struct FolderWalletInfo {
        QDir walletPath;
        QString nameWallet;

        FolderWalletInfo(const QDir &walletPath, const QString &nameWallet)
            : walletPath(walletPath)
            , nameWallet(nameWallet)
        {}
    };

    std::vector<FolderWalletInfo> folderWalletsInfos;

    QFileSystemWatcher fileSystemWatcher;

};

#endif // JAVASCRIPTWRAPPER_H
