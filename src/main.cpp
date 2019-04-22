#include "mainwindow.h"

#ifndef _WIN32
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include <iostream>

#include <QSurfaceFormat>
#include <QGuiApplication>
#include <QApplication>
#include <QCommandLineParser>

#include "RunGuard.h"

#include "check.h"
#include "Log.h"
#include "platform.h"
#include "tests.h"

#include "machine_uid.h"
#include "openssl_wrapper/openssl_wrapper.h"

#include "StopApplication.h"
#include "TypedException.h"
#include "Paths.h"
#include "NetwrokTesting.h"

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
#include "Initializer/Inits/InitMessenger.h"
#include "Initializer/Inits/InitWalletsNames.h"

#include "Module.h"
#include "proxy/Proxy.h"

#include "WalletRsa.h"

#include "MhPayEventHandler.h"
#include <QDebug>
#include <QProcess>
#include <QDir>

#ifndef _WIN32

static void crash_handler(int sig) {
    void *array[50];
    const size_t size = backtrace(array, 50);

    fprintf(stdout, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDOUT_FILENO);
    fflush(stdout);
    signal(SIGINT, nullptr);
    exit(1);
}
#endif

int main(int argc, char *argv[]) {
#ifndef _WIN32
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
#endif

    qputenv("QT_BEARER_POLL_TIMEOUT", QByteArray::number(-1)); // Эта установка дает warning QObject::startTimer: Timers cannot have negative intervals. Это нормально

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QSurfaceFormat format;
    format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("MetaGate");
    QCoreApplication::setApplicationVersion(QStringLiteral(VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription("MetaGate");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", QCoreApplication::translate("main", "Open url."));

    QCommandLineOption startintrayOption(QStringList() << "t" << "startintray",
            QCoreApplication::translate("main", "Hide MetaGate window."));
    parser.addOption(startintrayOption);

    QCommandLineOption debugPortOption(QStringList() <<
                                          "remote-debugging-port",
                                          "WebEngine debug port [default: 3002].",
                                          "remote-debug-port", "3002");
    parser.addOption(debugPortOption);

    parser.process(app);
    const QStringList args = parser.positionalArguments();
    const bool hide = parser.isSet(startintrayOption);

    std::string supposedMhPayUrl;
    if (!args.isEmpty()) {
        supposedMhPayUrl = args[0].toStdString();
    }

    RunGuard guard("MetaGate");
    if (!guard.tryToRun()) {
        std::cout << "Programm already running" << std::endl;
        if (!supposedMhPayUrl.empty()) {
            guard.storeValue(supposedMhPayUrl);
        }
        return 0;
    }

#ifdef Q_OS_MACX
    QProcess proc;
    QDir dir(qApp->applicationDirPath());
    proc.start("/bin/sh", QStringList() << dir.filePath(QStringLiteral("install.sh")));
#endif

    try {
        /*for (int i = 1; i < argc; i++) {
            if (argv[i] == std::string("--version")) {
                std::cout << VERSION_STRING << std::endl;
                return 0;
            }
        }*/

        MhPayEventHandler mhPayEventHandler(guard);
        app.installEventFilter(&mhPayEventHandler);
        initLog();
        InitOpenSSL();
        initializeAllPaths();
        initializeMachineUid();
        initModules();

        if (!supposedMhPayUrl.empty()) {
            mhPayEventHandler.processCommandLine(QString::fromStdString(supposedMhPayUrl));
        }

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
        LOG << "Is virtual machine " << isVirtualMachine();

        NetwrokTesting nettesting;
        nettesting.start();

        initializer::InitializerJavascript initJavascript;
        initializer::Initializer initManager(initJavascript);
        initJavascript.setInitializerManager(initManager);

        using namespace initializer;

        const std::shared_future<InitMainWindow::Return> mainWindow = initManager.addInit<InitMainWindow, true>(std::ref(initJavascript), versionString, typeString, GIT_CURRENT_SHA1, std::ref(mhPayEventHandler), hide);
        mainWindow.get(); // Сразу делаем здесь получение, чтобы инициализация происходила в этом потоке

        const std::shared_future<InitAuth::Return> auth = initManager.addInit<InitAuth>(mainWindow);

        const std::shared_future<InitNsLookup::Return> nsLookup = initManager.addInit<InitNsLookup>();

        const std::shared_future<InitTransactions::Return> transactions = initManager.addInit<InitTransactions>(mainWindow, nsLookup);

        const std::shared_future<InitWebSocket::Return> webSocketClient = initManager.addInit<InitWebSocket>();

        const std::shared_future<InitJavascriptWrapper::Return> jsWrapper = initManager.addInit<InitJavascriptWrapper>(webSocketClient, nsLookup, mainWindow, transactions, auth, QString::fromStdString(versionString));

        const std::shared_future<InitUploader::Return> uploader = initManager.addInit<InitUploader>(mainWindow, auth);

        //addModule(proxy::Proxy::moduleName());
        //const std::shared_future<InitProxy::Return> proxy = initManager.addInit<InitProxy>(mainWindow);

        const std::shared_future<InitMessenger::Return> messenger = initManager.addInit<InitMessenger>(mainWindow, auth, transactions, jsWrapper);

        const std::shared_future<InitWalletsNames::Return> walletNames = initManager.addInit<InitWalletsNames>(mainWindow, jsWrapper, auth, webSocketClient);

        initManager.complete();

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
