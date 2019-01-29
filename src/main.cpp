#include "mainwindow.h"

#ifndef _WIN32
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include <iostream>

#include <QSurfaceFormat>

#include "RunGuard.h"

#include "check.h"
#include "Log.h"
#include "platform.h"
#include "tests.h"
#include "utils.h"
#include "algorithms.h"

#include "machine_uid.h"
#include "openssl_wrapper/openssl_wrapper.h"

#include "StopApplication.h"
#include "TypedException.h"
#include "Paths.h"

#include "Initializer/Initializer.h"
#include "Initializer/InitializerJavascript.h"
#include "Initializer/Inits/InitMainwindow.h"
#include "Initializer/Inits/InitAuth.h"
#include "Initializer/Inits/InitNsLookup.h"
#include "Initializer/Inits/InitTransactions.h"
#include "Initializer/Inits/InitWebSocket.h"
#include "Initializer/Inits/InitJavascriptWrapper.h"
#include "Initializer/Inits/InitUploader.h"
#include "Initializer/Inits/InitProxy.h"

#include "Messenger/Messenger.h"
#include "Messenger/MessengerJavascript.h"
#include "Messenger/MessengerDBStorage.h"
#include "Messenger/CryptographicManager.h"

#include "Module.h"
#include "proxy/Proxy.h"

#ifndef _WIN32
static void crash_handler(int sig) {
    void *array[50];
    const size_t size = backtrace(array, 50);

    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    signal(SIGINT, nullptr);
    exit(1);
}
#endif

int main(int argc, char *argv[]) {
#ifndef _WIN32
    signal(SIGSEGV, crash_handler);
#endif

    RunGuard guard("MetaGate");
    if (!guard.tryToRun()) {
        std::cout << "Programm already running" << std::endl;
        return 0;
    }

    try {
        qputenv("QT_BEARER_POLL_TIMEOUT", QByteArray::number(-1));

        for (int i = 1; i < argc; i++) {
            if (argv[i] == std::string("--version")) {
                std::cout << VERSION_STRING << std::endl;
                return 0;
            }
        }

        QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
        QSurfaceFormat format;
        format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
        QSurfaceFormat::setDefaultFormat(format);

        QApplication app(argc, argv);
        initLog();
        InitOpenSSL();
        initializeAllPaths();
        initializeMachineUid();
        initModules();

        /*tests2();
        return 0;*/

        const std::string versionString = VERSION_STRING;

        std::string typeString;
        if (isProductionSetup) {
            typeString = "prod";
        } else {
            typeString = "dev";
        }

        LOG << "Version " << versionString << " " << typeString << " " << GIT_CURRENT_SHA1;
        LOG << "Platform " << osName;

        LOG << "Machine uid " << getMachineUid();

        initializer::InitializerJavascript initJavascript;
        initializer::Initializer initManager(initJavascript);
        initJavascript.setInitializerManager(initManager);

        using namespace initializer;

        const std::shared_future<InitMainWindow::Return> mainWindow = initManager.addInit<InitMainWindow, true>(std::ref(initJavascript), versionString, typeString, GIT_CURRENT_SHA1);
        mainWindow.get(); // Сразу делаем здесь получение, чтобы инициализация происходила в этом потоке

        const std::shared_future<InitAuth::Return> auth = initManager.addInit<InitAuth>(mainWindow);

        const std::shared_future<InitNsLookup::Return> nsLookup = initManager.addInit<InitNsLookup>();

        const std::shared_future<InitTransactions::Return> transactions = initManager.addInit<InitTransactions>(mainWindow, nsLookup);

        const std::shared_future<InitWebSocket::Return> webSocketClient = initManager.addInit<InitWebSocket>();

        const std::shared_future<InitJavascriptWrapper::Return> jsWrapper = initManager.addInit<InitJavascriptWrapper>(webSocketClient, nsLookup, mainWindow, transactions, auth, QString::fromStdString(versionString));

        const std::shared_future<InitUploader::Return> uploader = initManager.addInit<InitUploader>(mainWindow);

        addModule(proxy::Proxy::moduleName());
        const std::shared_future<InitProxy::Return> proxy = initManager.addInit<InitProxy>(webSocketClient, mainWindow);

        initManager.complete();
       
        /*
        messenger::CryptographicManager messengerCryptManager;
        messenger::MessengerJavascript messengerJavascript(authManager, jsWrapper, messengerCryptManager);
        emit mainWindow.setMessengerJavascript(messengerJavascript);
        messenger::MessengerDBStorage dbMessenger(getDbPath());
        dbMessenger.init();
        messenger::Messenger messenger(messengerJavascript, dbMessenger);
        messenger.start();
        messengerJavascript.setMessenger(messenger);
        */

        const int returnCode = app.exec();
        LOG << "Return code " << returnCode;
        return 0;
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (const TypedException &e) {
        LOG << "Error typed " << e.description;
    } catch (...) {
        LOG << "Unknown error";
    }

    return -1;
}
