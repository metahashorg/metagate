#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <vector>
#include <map>
#include <future>

#include "client.h"
#include "TypedException.h"

#include <QThread>

namespace initializer {

class InitInterface;
class InitializerJavascript;

struct InitState {
    int number;
    QString type;
    QString subType;
    QString message;
    TypedException exception;

    InitState() = default;

    InitState(int number, const QString &type, const QString &subType, const QString &message, const TypedException &exception);
};

class Initializer: public QObject {
    Q_OBJECT
public:

    using GetAllStatesCallback = std::function<void(const TypedException &exception)>;

    using ReadyCallback = std::function<void(const TypedException &exception)>;

    using Callback = std::function<void()>;

public:

    explicit Initializer(InitializerJavascript &javascriptWrapper, QObject *parent = nullptr);

    ~Initializer();

    void complete();

public:

    template<class Init, bool isDefferred, typename... Args>
    std::shared_future<typename Init::Return> addInit(Args&& ...args) {
        const int countStates = Init::countEvents();
        std::unique_ptr<Init> result = std::make_unique<Init>(QThread::currentThread(), *this, totalStates, totalStates + countStates);
        totalStates += countStates;
        auto fut = std::async((isDefferred ? std::launch::deferred : std::launch::async), &Init::initialize, result.get(), std::forward<Args>(args)...);
        initializiers.emplace_back(std::move(result));
        return fut;
    }

public:

    void sendState(const InitState &state);

private:

    template<typename Func>
    void runCallback(const Func &callback);

private:

    void sendStateToJs(const InitState &state);

    void sendInitializedToJs();

signals:

    void resendAllStatesSig(const GetAllStatesCallback &callback);

    void javascriptReadySig(const ReadyCallback &callback);

private slots:

    void onResendAllStates(const GetAllStatesCallback &callback);

    void onJavascriptReady(const ReadyCallback &callback);

private:

    InitializerJavascript &javascriptWrapper;

private:

    int totalStates = 0;

    std::map<int, InitState> states;

    bool isInitFinished = false;

    bool isComplete = false;

    std::vector<std::unique_ptr<InitInterface>> initializiers;
};

}

#endif // INITIALIZER_H
