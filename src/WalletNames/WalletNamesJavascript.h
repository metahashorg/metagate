#ifndef WALLETNAMESJAVASCRIPT_H
#define WALLETNAMESJAVASCRIPT_H

#include <functional>

#include <QObject>

#include "qt_utilites/WrapperJavascript.h"

namespace wallet_names {

class WalletNames;

class WalletNamesJavascript: public WrapperJavascript {
    Q_OBJECT

public:

    explicit WalletNamesJavascript(WalletNames& walletNames, QObject *parent = nullptr);

public slots:

    Q_INVOKABLE void saveKeyName(QString address, QString name);

    Q_INVOKABLE void getKeyName(QString address);

    Q_INVOKABLE void getAllWalletsInCurrency(QString currency);

    Q_INVOKABLE void advanceFill(QString jsonNames);

private slots:

    void onUpdatedWalletName(const QString &address, const QString &name);

    void onWalletsFlushed();

private:

    WalletNames &manager;

};

} // namespace wallet_names

#endif // WALLETNAMESJAVASCRIPT_H
