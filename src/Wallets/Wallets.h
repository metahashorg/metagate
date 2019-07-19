#ifndef WALLETS_WALLETS_H
#define WALLETS_WALLETS_H

#include "qt_utilites/TimerClass.h"

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "WalletInfo.h"

#include <QDir>

#include <QFileSystemWatcher>

namespace auth {
class Auth;
}

namespace wallets {

class Wallets: public ManagerWrapper, public TimerClass {
    Q_OBJECT
public:

    enum class WalletCurrency {
        Mth, Tmh, Btc, Eth
    };

public:

    using WalletsListCallback = CallbackWrapper<void(const QString &userName, const std::vector<WalletInfo> &walletAddresses)>;

public:

    explicit Wallets(auth::Auth &auth, QObject *parent = nullptr);

    ~Wallets() override;

public:

    const QString walletDefaultPath;

    const static QString defaultUsername;

signals:

    void getListWallets(const Wallets::WalletCurrency &type, const WalletsListCallback &callback);

private slots:

    void onGetListWallets(const Wallets::WalletCurrency &type, const WalletsListCallback &callback);

private slots:

    void onLogined(bool isInit, const QString &login, const QString &token);

    void onDirChanged(const QString &dir);

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

private:

    struct FolderWalletInfo {
        QDir walletPath;
        QString nameWallet;

        FolderWalletInfo(const QDir &walletPath, const QString &nameWallet)
            : walletPath(walletPath)
            , nameWallet(nameWallet)
        {}
    };

private:

    void setPathsImpl(QString newPatch, QString newUserName);

private:

    QString walletPath;

    QString userName;

    QString token;

    std::vector<FolderWalletInfo> folderWalletsInfos;

    QFileSystemWatcher fileSystemWatcher;

};

} // namespace wallets

#endif // WALLETS_WALLETS_H
