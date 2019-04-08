#ifndef INIT_WALLET_NAMES_H
#define INIT_WALLET_NAMES_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

struct TypedException;

class JavascriptWrapper;

namespace wallet_names {
class WalletNamesJavascript;
class WalletNames;
class WalletNamesDbStorage;
}

class MainWindow;

namespace initializer {

class InitializerJavascript;

class InitWalletsNames: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<wallet_names::WalletNamesJavascript*, wallet_names::WalletNames*>;

    using Callback = std::function<void()>;

public:

    InitWalletsNames(QThread *mainThread, Initializer &manager);

    ~InitWalletsNames() override;

    void completeImpl() override;

    Return initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<JavascriptWrapper*> jsWrap);

    static int countEvents() {
        return 1;
    }

    static int countCriticalEvents() {
        return 1;
    }

    static QString stateName();

private:

    void sendInitSuccess(const TypedException &exception);

signals:

    void callbackCall(const InitWalletsNames::Callback &callback);

private slots:

    void onCallbackCall(const InitWalletsNames::Callback &callback);

private:

    std::unique_ptr<wallet_names::WalletNamesJavascript> javascript;
    std::unique_ptr<wallet_names::WalletNames> manager;
    std::unique_ptr<wallet_names::WalletNamesDbStorage> database;

};

}

#endif // INIT_WALLET_NAMES_H
