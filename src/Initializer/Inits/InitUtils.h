#ifndef INIT_UTILS_H
#define INIT_UTILS_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

struct TypedException;

namespace utils {
class Utils;
class UtilsJavascript;
}

class MainWindow;

namespace initializer {

class InitializerJavascript;

class InitUtils: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<utils::Utils*, utils::UtilsJavascript*>;

    using Callback = std::function<void()>;

public:

    InitUtils(QThread *mainThread, Initializer &manager);

    ~InitUtils() override;

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

    void callbackCall(const InitUtils::Callback &callback);

private slots:

    void onCallbackCall(const InitUtils::Callback &callback);

private:

    std::unique_ptr<utils::Utils> utilsManager;
    std::unique_ptr<utils::UtilsJavascript> utilsJavascript;

};

}

#endif // INIT_UTILS_H
