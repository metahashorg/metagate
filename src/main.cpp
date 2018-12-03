#include "mainwindow.h"

#ifndef _WIN32
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include <iostream>

#include <QSurfaceFormat>
#include <QSettings>

#include "RunGuard.h"

#include "check.h"
#include "Log.h"
#include "platform.h"
#include "tests.h"
#include "utils.h"

#include "machine_uid.h"
#include "openssl_wrapper/openssl_wrapper.h"

#include "uploader.h"
#include "NsLookup.h"
#include "StopApplication.h"
#include "WebSocketClient.h"
#include "JavascriptWrapper.h"
#include "TypedException.h"
#include "Paths.h"

#include "auth/Auth.h"
#include "auth/AuthJavascript.h"

#include "Messenger/Messenger.h"
#include "Messenger/MessengerJavascript.h"
#include "Messenger/MessengerDBStorage.h"

#include "transactions/Transactions.h"
#include "transactions/TransactionsJavascript.h"
#include "transactions/TransactionsDBStorage.h"

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

QString getUrlToWss() {
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("web_socket/meta_online"), "web_socket/meta_online not found setting");
    return settings.value("web_socket/meta_online").toString();
}

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

        /*messenger::MessengerDBStorage dbMessenger(getDbPath());
        dbMessenger.init();*/

        auth::AuthJavascript authJavascript;
        auth::Auth authManager(authJavascript);
        authManager.start();

        NsLookup nsLookup;
        nsLookup.start();

        transactions::TransactionsDBStorage dbTransactions(getDbPath());
        dbTransactions.init();
        transactions::TransactionsJavascript transactionsJavascript;
        transactions::Transactions transactionsManager(nsLookup, transactionsJavascript, dbTransactions);
        transactionsManager.start();

        WebSocketClient webSocketClient(getUrlToWss());
        webSocketClient.start();

        JavascriptWrapper jsWrapper(webSocketClient, nsLookup, transactionsManager, authManager, QString::fromStdString(versionString));

        messenger::MessengerJavascript messengerJavascript(authManager, jsWrapper);
        /*messenger::Messenger messenger(messengerJavascript, dbMessenger);
        messenger.start();
        messengerJavascript.setMessenger(messenger);*/

        MainWindow mainWindow(jsWrapper, authJavascript, messengerJavascript, transactionsJavascript, authManager);
        mainWindow.showExpanded();

        mainWindow.setWindowTitle(APPLICATION_NAME + QString::fromStdString(" -- " + versionString + " " + typeString + " " + GIT_CURRENT_SHA1));

        Uploader uploader(&mainWindow);
        uploader.start();

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
