#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <vector>
#include <map>
#include <future>
#include <set>

#include "TypedException.h"
#include "check.h"

#include <QThread>

namespace initializer {

class InitInterface;
class InitializerJavascript;

struct InitState {
    QString type;
    QString subType;
    QString message;
    TypedException exception;

    InitState() = default;

    InitState(const QString &type, const QString &subType, const QString &message, const TypedException &exception);
};

class Initializer: public QObject {
    Q_OBJECT
public:

    enum class ReadyType {
        Error, Advance, Finish
    };

    using GetAllStatesCallback = std::function<void(const TypedException &exception)>;

    using ReadyCallback = std::function<void(const ReadyType &result, const TypedException &exception)>;

    using Callback = std::function<void()>;

public:

    explicit Initializer(InitializerJavascript &javascriptWrapper, QObject *parent = nullptr);

    ~Initializer();

    void complete();

public:

    template<class Init, bool isDefferred, typename... Args>
    std::shared_future<typename Init::Return> addInit(Args&& ...args) {
        CHECK(!isComplete, "Already complete");
        totalStates += Init::countEvents();
        std::unique_ptr<Init> result = std::make_unique<Init>(QThread::currentThread(), *this);
        auto fut = std::async((isDefferred ? std::launch::deferred : std::launch::async), &Init::initialize, result.get(), std::forward<Args>(args)...);
        initializiers.emplace_back(std::move(result));
        return fut;
    }

signals:

    void sendState(const InitState &state);

private slots:

    void onSendState(const InitState &state);

private:

    template<typename Func>
    void runCallback(const Func &callback);

private:

    void sendStateToJs(const InitState &state, int number);

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

    std::vector<InitState> states;

    std::set<std::pair<QString, QString>> existStates;

    bool isInitFinished = false;

    bool isComplete = false;

    std::vector<std::unique_ptr<InitInterface>> initializiers;
};

}

#endif // INITIALIZER_H
