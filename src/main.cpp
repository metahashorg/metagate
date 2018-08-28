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

#include "machine_uid.h"
#include "openssl_wrapper/openssl_wrapper.h"

#include "uploader.h"
#include "NsLookup.h"
#include "StopApplication.h"
#include "WebSocketClient.h"
#include "JavascriptWrapper.h"
#include "TypedException.h"
#include "Paths.h"

#include "Messenger/Messenger.h"
#include "Messenger/MessengerJavascript.h"
#include "Messenger/messengerdbstorage.h"

#include "transactions/Transactions.h"
#include "transactions/TransactionsJavascript.h"
#include "transactions/transactionsdbstorage.h"

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
    const static QString WEB_SOCKET_SERVER_FILE = "web_socket.txt";

    const QString pathToWebSServer = makePath(getSettingsPath(), WEB_SOCKET_SERVER_FILE);
    const std::string &fileData = readFile(pathToWebSServer);
    return QString::fromStdString(fileData).trimmed();
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
        qRegisterMetaType<ReturnCallback>("ReturnCallback");
        qRegisterMetaType<WindowEvent>("WindowEvent");

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
        //app.setApplicationDisplayName(QString::fromStdString(versionString + " " + typeString + " " + GIT_CURRENT_SHA1));
        LOG << "Platform " << osName;

        LOG << "Machine uid " << getMachineUid();

        MessengerDBStorage dbMessenger(getDbPath());
        dbMessenger.init();

        transactions::TransactionsDBStorage dbTransactions(getDbPath());
        dbTransactions.init();
        while (true) {
            MessengerJavascript messengerJavascript;

            Messenger messenger(messengerJavascript, dbMessenger);
            messenger.start();
            messengerJavascript.setMessenger(messenger);

            NsLookup nsLookup(getSettingsPath());
            nsLookup.start();

            transactions::TransactionsJavascript transactionsJavascript;

            transactions::Transactions transactionsManager(nsLookup, transactionsJavascript, dbTransactions);
            transactionsManager.start();
            transactionsJavascript.setTransactions(transactionsManager);

            WebSocketClient webSocketClient(getUrlToWss());
            webSocketClient.start();

            JavascriptWrapper jsWrapper(webSocketClient, nsLookup, transactionsManager, QString::fromStdString(versionString));

            MainWindow mainWindow(jsWrapper, messengerJavascript, transactionsJavascript);
            mainWindow.showExpanded();

            mainWindow.setWindowTitle(APPLICATION_NAME + QString::fromStdString(" -- " + versionString + " " + typeString + " " + GIT_CURRENT_SHA1));

            Uploader uploader(&mainWindow);
            uploader.start();

            const int returnCode = app.exec();
            LOG << "Return code " << returnCode;
            if (returnCode != RESTART_BROWSER) {
                break;
            }
        }

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
