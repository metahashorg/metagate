#ifndef INITIALIZER_JAVASCRIPT_H
#define INITIALIZER_JAVASCRIPT_H

#include <QObject>
#include <functional>

struct TypedException;

namespace initializer {

class Initializer;

struct InitState;

class InitializerJavascript : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void()>;

public:
    explicit InitializerJavascript(QObject *parent = nullptr);

    void setInitializerManager(Initializer &initializer) {
        m_initializer = &initializer;
    }

public slots:

    Q_INVOKABLE void resendEvents();

    Q_INVOKABLE void ready(bool force);

    Q_INVOKABLE void getAllTypes();

    Q_INVOKABLE void getAllSubTypes();

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const Callback &callback);

public slots:

    void onCallbackCall(const Callback &callback);

signals:

    void stateChangedSig(int number, int totalStates, int numberCritical, int totalCritical, const InitState &state);

    void initializedSig(bool isSuccess, const TypedException &exception);

    void initializedCriticalSig(bool isSuccess, const TypedException &exception);

private slots:

    void onStateChanged(int number, int totalStates, int numberCritical, int totalCritical, const InitState &state);

    void onInitialized(bool isSuccess, const TypedException &exception);

    void onInitializedCritical(bool isSuccess, const TypedException &exception);

private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:

    Initializer *m_initializer;

    std::function<void(const std::function<void()> &callback)> signalFunc;
};

}
#endif // INITIALIZER_JAVASCRIPT_H
