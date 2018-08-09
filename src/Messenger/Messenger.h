#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObject>

#include "TimerClass.h"
#include "WebSocketClient.h"

#include <map>

class Messenger : public TimerClass
{
    Q_OBJECT
public:

    using Counter = uint64_t;

private:

    class DeferredMessage {
    public:

        bool check() const {
            if (!isDeferred_) {
                return false;
            }
            const time_point now = ::now();
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(now - begin_);
            return duration >= elapse_;
        }

        bool isDeferred() const {
            return isDeferred_;
        }

        void setDeferred(const milliseconds &elapse) {
            isDeferred_ = true;
            begin_ = ::now();
            elapse_ = elapse;
        }

        void resetDeferred() {
            isDeferred_ = false;
        }

    private:
        bool isDeferred_ = false;
        time_point begin_;
        milliseconds elapse_;
    };

public:

    struct NewMessageResponse {
        QString data;
        bool isInput;
        Messenger::Counter counter;

        bool operator< (const NewMessageResponse &second) const {
            return this->counter < second.counter;
        }
    };

public:

    explicit Messenger(QObject *parent = nullptr);

    static std::vector<QString> stringsForSign();

signals:

    void registerAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee);

    void getPubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex);

    void sendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const QString &encryptedDataHex);

    void signedStrings(const std::vector<QString> &signedHexs);

private slots:

    void onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee);

    void onGetPubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex);

    void onSendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const QString &encryptedDataHex);

    void onSignedStrings(const std::vector<QString> &signedHexs);

private slots:

    void onRun();

    void onTimerEvent();

    void onWssMessageReceived(QString message);

private:

    void getMessagesFromAddressFromWss(const QString &fromAddress, Counter from, Counter to);

    void clearAddressesToMonitored();

    void addAddressToMonitored(const QString &address);

    void processMessages(const QString &address, const std::vector<NewMessageResponse> &messages);

    QString getPublicKeyFromMethod(const QString &address, const QString &method) const;

    std::vector<QString> getMonitoredAddresses() const;

private:

    WebSocketClient wssClient;

    std::map<QString, DeferredMessage> deferredMessages;

};

#endif // MESSENGER_H
