#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <vector>
#include <map>
#include <future>
#include <set>

#include "TypedException.h"
#include "check.h"
#include "CallbackWrapper.h"

#include <QString>
#include <QThread>

namespace initializer {

class InitInterface;
class InitializerJavascript;

struct InitState {
    QString type;
    QString subType;
    QString message;
    bool isCritical;
    bool isScipped;
    TypedException exception;

    InitState() = default;

    InitState(const QString &type, const QString &subType, const QString &message, bool isCritical, bool isScipped, const TypedException &exception);
};

class Initializer: public QObject {
    Q_OBJECT
public:

    enum class ReadyType {
        Error, CriticalAdvance, Advance, Finish, NotSuccess
    };

    struct StateType {
        QString type;
        QString subtype;
        QString message;
        bool isCritical;

        StateType(const QString &type, const QString &subtype, const QString &message, bool isCritical)
            : type(type)
            , subtype(subtype)
            , message(message)
            , isCritical(isCritical)
        {}
    };

public:

    using GetAllStatesCallback = CallbackWrapper<void()>;

    using ReadyCallback = CallbackWrapper<void(const ReadyType &result)>;

    using GetTypesCallback = CallbackWrapper<void(const std::vector<QString> &result)>;

    using GetSubTypesCallback = CallbackWrapper<void(const std::vector<StateType> &result)>;

    using Callback = std::function<void()>;

public:

    explicit Initializer(InitializerJavascript &javascriptWrapper, QObject *parent = nullptr);

    ~Initializer();

    void complete();

public:

    template<class Init, bool isDefferred=false, typename... Args>
    std::shared_future<typename Init::Return> addInit(Args&& ...args) {
        CHECK(!isCompleteSets, "Already complete initializer");

        totalStates += Init::countEvents();
        totalCriticalStates += Init::countCriticalEvents();

        CHECK(countStatesForInits.find(Init::stateName()) == countStatesForInits.end(), "Conflict name: " + Init::stateName().toStdString() + " already exist");
        countStatesForInits[Init::stateName()] = Init::countEvents();
        CHECK(countCriticalStatesForInits.find(Init::stateName()) == countCriticalStatesForInits.end(), "Conflict name: " + Init::stateName().toStdString() + " already exist");
        countCriticalStatesForInits[Init::stateName()] = Init::countCriticalEvents();

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

    void sendStateToJs(const InitState &state, int number, int numberCritical);

    void sendInitializedToJs(bool isErrorExist);

    void sendCriticalInitializedToJs(bool isErrorExist);

signals:

    void resendAllStatesSig(const GetAllStatesCallback &callback);

    void javascriptReadySig(bool force, const ReadyCallback &callback);

    void getAllTypes(const GetTypesCallback &callback);

    void getAllSubTypes(const GetSubTypesCallback &callback);

private slots:

    void onResendAllStates(const GetAllStatesCallback &callback);

    void onJavascriptReady(bool force, const ReadyCallback &callback);

    void onGetAllTypes(const GetTypesCallback &callback);

    void onGetAllSubTypes(const GetSubTypesCallback &callback);

private:

    InitializerJavascript &javascriptWrapper;

private:

    int totalStates = 0;

    int totalCriticalStates = 0;

    int countCritical = 0;

    std::vector<InitState> states;

    std::set<std::pair<QString, QString>> existStates;

    bool isInitFinished = false;

    bool isCriticalInitFinished = false;

    bool isCompleteSets = false;

    std::vector<std::unique_ptr<InitInterface>> initializiers;

    bool isErrorExist = false;

    bool isErrorCritical = false;

    std::map<QString, int> countStatesForInits;

    std::map<QString, int> countCriticalStatesForInits;
};

}

#endif // INITIALIZER_H
