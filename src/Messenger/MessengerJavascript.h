#ifndef MESSENGERJAVASCRIPT_H
#define MESSENGERJAVASCRIPT_H

#include <QObject>

#include <functional>

#include "Message.h"

#include "qt_utilites/WrapperJavascript.h"

namespace auth {
class Auth;
}

namespace transactions {
class Transactions;
}

namespace wallets {
class Wallets;
}

namespace messenger {

class CryptographicManager;

class Messenger;

class MessengerJavascript: public WrapperJavascript {
    Q_OBJECT

public:
    explicit MessengerJavascript(auth::Auth &authManager, CryptographicManager &cryptoManager, transactions::Transactions &txManager, wallets::Wallets &wallets);

    void setMessenger(Messenger &m);

public slots:
    void onLogined(bool isInit, const QString login);

    void onMthWalletCreated(bool isMhc, const QString &name, const QString &userName);

signals:

    void newMessegesSig(QString address, Message::Counter lastMessage);

    void addedToChannelSig(QString address, QString title, QString titleSha, QString admin, Message::Counter counter);

    void deletedFromChannelSig(QString address, QString title, QString titleSha, QString admin);

    void newMessegesChannelSig(QString address, QString titleSha, Message::Counter lastMessage);

    void requiresPubkeySig(QString address, QString collocutor);

    void collocutorAddedPubkeySig(QString address, QString collocutor);

private slots:

    void onNewMesseges(QString address, Message::Counter lastMessage);

    void onAddedToChannel(QString address, QString title, QString titleSha, QString admin, Message::Counter counter);

    void onDeletedFromChannel(QString address, QString title, QString titleSha, QString admin);

    void onNewMessegesChannel(QString address, QString titleSha, Message::Counter lastMessage);

    void onRequiresPubkey(QString address, QString collocutor);

    void onCollocutorAddedPubkey(QString address, QString collocutor);

public slots:

    Q_INVOKABLE void getHistoryAddress(QString address, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to);

    Q_INVOKABLE void sendPubkeyAddressToBlockchain(QString address, QString feeStr, QString paramsJson);

    Q_INVOKABLE void registerAddress(bool isForcibly, QString address, QString feeStr);

    Q_INVOKABLE void registerAddressBlockchain(bool isForcibly, QString address, QString feeStr, QString txHash, QString blockchainName, QString blockchainServ);

    Q_INVOKABLE void savePublicKeyCollocutor(bool isForcibly, QString address, QString collocutor);

    Q_INVOKABLE void getUserInfo(QString address);

    Q_INVOKABLE void getCollocutorInfo(QString address);

    Q_INVOKABLE void sendMessage(QString address, QString collocutor, QString dataHex, QString timestampStr, QString feeStr);

    Q_INVOKABLE void getLastMessageNumber(QString address);

    Q_INVOKABLE void getSavedPos(QString address, const QString &collocutor);

    Q_INVOKABLE void getSavedsPos(QString address);

    Q_INVOKABLE void savePos(QString address, const QString &collocutor, QString counterStr);

    Q_INVOKABLE void getCountMessages(QString address, const QString &collocutor, QString from);


    Q_INVOKABLE void createChannel(QString address, QString channelTitle, QString fee);

    Q_INVOKABLE void addWriterToChannel(QString address, QString titleSha, QString writer);

    Q_INVOKABLE void delWriterFromChannel(QString address, QString titleSha, QString writer);

    Q_INVOKABLE void sendMessageToChannel(QString address, QString titleSha, QString dataHex, QString timestampStr, QString feeStr);

    Q_INVOKABLE void getChannelsList(QString address);

    Q_INVOKABLE void getLastMessageChannelNumber(QString address, QString titleSha);

    Q_INVOKABLE void getSavedPosChannel(QString address, QString titleSha);

    Q_INVOKABLE void savePosToChannel(QString address, const QString &titleSha, QString counterStr);

    Q_INVOKABLE void getHistoryAddressChannel(QString address, QString titleSha, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressChannelCount(QString address, QString titleSha, QString count, QString to);


    Q_INVOKABLE void setMhcType(bool isMhc);

    Q_INVOKABLE void unlockWallet(QString address, QString password, QString passwordRsa, int timeSeconds);

    Q_INVOKABLE void lockWallet();

    Q_INVOKABLE void remainingTime();


    Q_INVOKABLE void reEmit();

private:

    void setPathsImpl();

    void doRegisterAddress(const QString &address, bool isNew, bool isForcibly, const std::function<void(const TypedException &exception, const QString &result)> &makeFunc, const std::function<void(const TypedException &exception)> &errorFunc);

private:

    auth::Auth &authManager;

    Messenger *messenger = nullptr;

    CryptographicManager &cryptoManager;

    transactions::Transactions &txManager;

    wallets::Wallets &wallets;

    QString currentUserName;

    bool isUserNameSetted = false;

    bool isMhc;

};

}

#endif // MESSENGERJAVASCRIPT_H
