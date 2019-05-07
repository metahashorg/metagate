#ifndef INIT_UPLOADER_H
#define INIT_UPLOADER_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>

class MainWindow;
class Uploader;
struct TypedException;

namespace auth {
class Auth;
class AuthJavascript;
}

namespace initializer {

class InitializerJavascript;

class InitUploader: public InitInterface {
    Q_OBJECT
public:

    using Return = Uploader*;

public:

    InitUploader(QThread *mainThread, Initializer &manager);

    ~InitUploader() override;

    void completeImpl() override;

    Return initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth);

    static int countEvents() {
        return 2;
    }

    static int countCriticalEvents() {
        return 1;
    }

    static QString stateName();

signals:

    void checkedUpdatesHtmls(const TypedException &exception);

private:

    void sendInitSuccess(const TypedException &exception);

private slots:

    void onCheckedUpdatesHtmls(const TypedException &exception);

private:

    std::unique_ptr<Uploader> uploader;

};

}

#endif // INIT_UPLOADER_H
