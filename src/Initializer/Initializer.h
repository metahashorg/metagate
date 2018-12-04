#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <vector>

#include "client.h"
#include "TypedException.h"

namespace initializer {

struct InitState {
    int number;
    QString type;
    QString subType;
    QString message;
    TypedException exception;

    InitState() = default;

    InitState(int number, const QString &type, const QString &subType, const QString &message, const TypedException &exception);
};

class InitializerJavascript;

class Initializer: public QObject {
    Q_OBJECT
public:

    using GetAllStatesCallback = std::function<void(const TypedException &exception)>;

    using ReadyCallback = std::function<void(const TypedException &exception)>;

    using Callback = std::function<void()>;

public:

    explicit Initializer(InitializerJavascript &javascriptWrapper, QObject *parent = nullptr);

private:

    template<typename Func>
    void runCallback(const Func &callback);

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

    bool isInitFinished = false;
};

}

#endif // INITIALIZER_H
