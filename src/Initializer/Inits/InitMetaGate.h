#ifndef INIT_METAGATE_H
#define INIT_METAGATE_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

struct TypedException;

class MainWindow;

namespace metagate {
class MetaGate;
class MetaGateJavascript;
}

namespace initializer {

class InitializerJavascript;

class InitMetaGate: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<metagate::MetaGate*, metagate::MetaGateJavascript*>;

    using Callback = std::function<void()>;

public:

    InitMetaGate(QThread *mainThread, Initializer &manager);

    ~InitMetaGate() override;

    void completeImpl() override;

    Return initialize(std::shared_future<MainWindow*> mainWindow);

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

    void callbackCall(const InitMetaGate::Callback &callback);

private slots:

    void onCallbackCall(const InitMetaGate::Callback &callback);

private:

    std::unique_ptr<metagate::MetaGate> manager;
    std::unique_ptr<metagate::MetaGateJavascript> javascript;

};

}

#endif // INIT_METAGATE_H
