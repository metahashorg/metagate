#include "mainwindow.h"

#ifndef _WIN32
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include <iostream>

#include "check.h"
#include "Log.h"
#include "platform.h"
#include "tests.h"
#include "tests2.h"

#include "machine_uid.h"
#include "openssl_wrapper/openssl_wrapper.h"

#include "uploader.h"
#include "NsLookup.h"
#include "StopApplication.h"
#include "WebSocketClient.h"
#include "JavascriptWrapper.h"

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

        QApplication app(argc, argv);

        initLog();
        InitOpenSSL();

        /*allTests();
        return 0;*/

        const std::string versionString = VERSION_STRING;

        std::string typeString;
        if (isProductionSetup) {
            typeString = "dev";
        } else {
            typeString = "prod";
        }

        LOG << "Version " << versionString << " " << typeString << " " << GIT_CURRENT_SHA1;
        //app.setApplicationDisplayName(QString::fromStdString(versionString + " " + typeString + " " + GIT_CURRENT_SHA1));
        LOG << "Platform " << osName;

        LOG << "Machine uid " << getMachineUid();

        while (true) {
            ServerName serverName;

            NsLookup nsLookup(Uploader::getPagesPath());
            nsLookup.start();

            WebSocketClient webSocketClient;
            webSocketClient.start();

            JavascriptWrapper jsWrapper(nsLookup);

            MainWindow mainWindow(webSocketClient, jsWrapper, QString::fromStdString(versionString));
            mainWindow.showExpanded();

            mainWindow.setWindowTitle(APPLICATION_NAME + QString::fromStdString(" -- " + versionString + " " + typeString + " " + GIT_CURRENT_SHA1));

            Uploader uploader(&mainWindow, serverName);
            uploader.start();

            const int returnCode = app.exec();
            if (returnCode != RESTART_BROWSER) {
                break;
            }
        }

        return 0;
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }

    return -1;
}
