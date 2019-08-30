TEMPLATE = app

TARGET = MetaGate

DEFINES += VERSION_STRING=\\\"1.20.0\\\"
DEFINES += VERSION_SETTINGS=\\\"10.5\\\"
#DEFINES += DEVELOPMENT
DEFINES += PRODUCTION
DEFINES += APPLICATION_NAME=\\\"MetaGate\\\"

DEFINES += GIT_CURRENT_SHA1="\\\"$$system(git rev-parse --short HEAD)\\\""

QMAKE_INFO_PLIST +=  ../deploy/mac/default.plist

SOURCES += main.cpp mainwindow.cpp \
    uploader.cpp \
    Wallets/ethtx/scrypt/crypto_scrypt-nosse.cpp \
    Wallets/ethtx/scrypt/sha256.cpp \
    Wallets/ethtx/cert.cpp \
    Wallets/ethtx/rlp.cpp \
    Wallets/ethtx/ethtx.cpp \
    Wallets/ethtx/cert2.cpp \
    Wallets/ethtx/scrypt/crypto_scrypt_saltgen.cpp \
    Wallets/ethtx/crossguid/Guid.cpp \
    Wallets/btctx/Base58.cpp \
    Wallets/btctx/btctx.cpp \
    Wallets/btctx/wif.cpp \
    StopApplication.cpp \
    tests.cpp \
    Wallets/openssl_wrapper/openssl_wrapper.cpp \
    Wallets/ethtx/utils2.cpp \
    NsLookup/NsLookup.cpp \
    NsLookup/dns/datatransformer.cpp \
    NsLookup/dns/dnspacket.cpp \
    NsLookup/dns/resourcerecord.cpp \
    JavascriptWrapper.cpp \
    PagesMappings.cpp \
    mhurlschemehandler.cpp \
    Paths.cpp \
    RunGuard.cpp \
    Messenger/Messenger.cpp \
    Messenger/MessengerMessages.cpp \
    Messenger/MessengerJavascript.cpp \
    Messenger/CryptographicManager.cpp \
    dbstorage.cpp \
    Messenger/MessengerDBStorage.cpp \
    transactions/Transactions.cpp \
    transactions/TransactionsMessages.cpp \
    transactions/TransactionsDBStorage.cpp \
    transactions/TransactionsJavascript.cpp \
    proxy/UPnPDevices.cpp \
    proxy/UPnPRouter.cpp \
    proxy/ProxyServer.cpp \
    proxy/ProxyClient.cpp \
    proxy/Proxy.cpp \
    proxy/ProxyJavascript.cpp \
    auth/Auth.cpp \
    auth/AuthJavascript.cpp \
    Initializer/Initializer.cpp \
    Initializer/InitializerJavascript.cpp \
    Initializer/InitInterface.cpp \
    Initializer/Inits/InitMainwindow.cpp \
    Initializer/Inits/InitAuth.cpp \
    Initializer/Inits/InitNsLookup.cpp \
    Initializer/Inits/InitTransactions.cpp \
    Initializer/Inits/InitWebSocket.cpp \
    Initializer/Inits/InitJavascriptWrapper.cpp \
    Initializer/Inits/InitUploader.cpp \
    Module.cpp \
    proxy/WebSocketSender.cpp \
    Initializer/Inits/InitProxy.cpp \
    Initializer/Inits/InitMessenger.cpp \
    MhPayEventHandler.cpp \
    WalletNames/WalletNamesDbStorage.cpp \
    WalletNames/WalletNames.cpp \
    WalletNames/WalletNamesJavascript.cpp \
    Initializer/Inits/InitWalletsNames.cpp \
    WalletNames/WalletNamesMessages.cpp \
    Utils/UtilsJavascript.cpp \
    Initializer/Inits/InitUtils.cpp \
    Utils/UtilsManager.cpp \
    NsLookup/TaskManager.cpp \
    NsLookup/NslWorker.cpp \
    NsLookup/Workers/FullWorker.cpp \
    NsLookup/Workers/SimpleWorker.cpp \
    NsLookup/Workers/RefreshIpWorker.cpp \
    NsLookup/Workers/RefreshNodeWorker.cpp \
    NsLookup/Workers/FindEmptyNodesWorker.cpp \
    NsLookup/Workers/PrintNodesWorker.cpp \
    NsLookup/Workers/MiddleWorker.cpp \
    utilites/BigNumber.cpp \
    utilites/machine_uid.cpp \
    utilites/machine_uid_unix.cpp \
    utilites/machine_uid_win.cpp \
    utilites/qrcoder.cpp \
    utilites/unzip.cpp \
    utilites/utils.cpp \
    utilites/VersionWrapper.cpp \
    Log.cpp \
    TypedException.cpp \
    qt_utilites/CallbackCallWrapper.cpp \
    qt_utilites/CallbackWrapper.cpp \
    qt_utilites/ManagerWrapper.cpp \
    qt_utilites/QRegister.cpp \
    qt_utilites/TimerClass.cpp \
    qt_utilites/WrapperJavascript.cpp \
    Network/SimpleClient.cpp \
    Network/HttpClient.cpp \
    Network/NetwrokTesting.cpp \
    Network/UdpSocketClient.cpp \
    Network/WebSocketClient.cpp \
    Wallets/BtcWallet.cpp \
    Wallets/EthWallet.cpp \
    Wallets/Wallet.cpp \
    Wallets/WalletRsa.cpp \
    Wallets/Wallets.cpp \
    Wallets/WalletsJavascript.cpp \
    Wallets/WalletInfo.cpp \
    Initializer/Inits/InitWallets.cpp \
    qt_utilites/EventWatcher.cpp \
    Wallets/GetActualWalletsEvent.cpp \
    NsLookup/InfrastructureNsLookup.cpp \
    Network/LocalServer.cpp

unix: SOURCES +=
SOURCES +=  proxy/http_parser.c

HEADERS += mainwindow.h \
    uploader.h \
    Wallets/ethtx/scrypt/libscrypt.h \
    Wallets/ethtx/scrypt/sha256.h \
    Wallets/ethtx/scrypt/sysendian.h \
    Wallets/ethtx/cert.h \
    Wallets/ethtx/const.h \
    Wallets/ethtx/rlp.h \
    Wallets/ethtx/ethtx.h \
    Wallets/ethtx/crossguid/Guid.hpp \
    Wallets/btctx/Base58.h \
    Wallets/btctx/btctx.h \
    Wallets/btctx/wif.h \
    Wallets/btctx/Base58.h \
    Wallets/btctx/btctx.h \
    Wallets/btctx/wif.h \
    StopApplication.h \
    tests.h \
    Wallets/openssl_wrapper/openssl_wrapper.h \
    Wallets/ethtx/utils2.h \
    NsLookup/NsLookup.h \
    NsLookup/dns/datatransformer.h \
    NsLookup/dns/dnspacket.h \
    NsLookup/dns/resourcerecord.h \
    JavascriptWrapper.h \
    PagesMappings.h \
    mhurlschemehandler.h \
    Paths.h \
    RunGuard.h \
    Messenger/Messenger.h \
    Messenger/MessengerMessages.h \
    Messenger/MessengerJavascript.h \
    Messenger/Message.h \
    dbstorage.h \
    Messenger/MessengerDBStorage.h \
    transactions/Transactions.h \
    transactions/TransactionsMessages.h \
    transactions/Transaction.h \
    transactions/TransactionsDBStorage.h \
    transactions/TransactionsJavascript.h \
    proxy/UPnPDevices.h \
    proxy/UPnPRouter.h \
    proxy/ProxyServer.h \
    proxy/ProxyClient.h \
    proxy/Proxy.h \
    proxy/ProxyJavascript.h \
    auth/Auth.h \
    auth/AuthJavascript.h \
    Initializer/Initializer.h \
    Initializer/InitializerJavascript.h \
    Initializer/InitInterface.h \
    Initializer/Inits/InitMainwindow.h \
    Initializer/Inits/InitAuth.h \
    Initializer/Inits/InitNsLookup.h \
    Initializer/Inits/InitTransactions.h \
    Initializer/Inits/InitWebSocket.h \
    Initializer/Inits/InitJavascriptWrapper.h \
    Initializer/Inits/InitUploader.h \
    Module.h \
    Messenger/CryptographicManager.h \
    proxy/WebSocketSender.h \
    Initializer/Inits/InitProxy.h \
    Initializer/Inits/InitMessenger.h \
    MhPayEventHandler.h \
    WalletNames/WalletNamesDbStorage.h \
    WalletNames/WalletNamesDbRes.h \
    WalletNames/WalletInfo.h \
    WalletNames/WalletNames.h \
    WalletNames/WalletNamesJavascript.h \
    Initializer/Inits/InitWalletsNames.h \
    WalletNames/WalletNamesMessages.h \
    Utils/UtilsJavascript.h \
    Initializer/Inits/InitUtils.h \
    Utils/UtilsManager.h \
    NsLookup/TaskManager.h \
    NsLookup/NslWorker.h \
    NsLookup/Workers/FullWorker.h \
    NsLookup/NsLookupStructs.h \
    NsLookup/Workers/SimpleWorker.h \
    NsLookup/Workers/RefreshIpWorker.h \
    NsLookup/Workers/RefreshNodeWorker.h \
    NsLookup/Workers/FindEmptyNodesWorker.h \
    NsLookup/Workers/PrintNodesWorker.h \
    NsLookup/Workers/MiddleWorker.h \
    utilites/algorithms.h \
    utilites/BigNumber.h \
    utilites/machine_uid.h \
    utilites/platform.h \
    utilites/qrcoder.h \
    utilites/RequestId.h \
    utilites/unzip.h \
    utilites/utils.h \
    utilites/VersionWrapper.h \
    check.h \
    Log.h \
    duration.h \
    TypedException.h \
    qt_utilites/CallbackCallWrapper.h \
    qt_utilites/CallbackWrapper.h \
    qt_utilites/makeJsFunc.h \
    qt_utilites/ManagerWrapper.h \
    qt_utilites/ManagerWrapperImpl.h \
    qt_utilites/QRegister.h \
    qt_utilites/SlotWrapper.h \
    qt_utilites/TimerClass.h \
    qt_utilites/WrapperJavascript.h \
    qt_utilites/WrapperJavascriptImpl.h \
    Network/SimpleClient.h \
    Network/HttpClient.h \
    Network/NetwrokTesting.h \
    Network/UdpSocketClient.h \
    Network/WebSocketClient.h \
    Wallets/BtcWallet.h \
    Wallets/EthWallet.h \
    Wallets/Wallet.h \
    Wallets/WalletRsa.h \
    Wallets/Wallets.h \
    Wallets/WalletsJavascript.h \
    Wallets/WalletInfo.h \
    Initializer/Inits/InitWallets.h \
    qt_utilites/EventWatcher.h \
    Wallets/GetActualWalletsEvent.h \
    transactions/TransactionsFilter.h \
    NsLookup/InfrastructureNsLookup.h \
    Network/LocalServer.h

FORMS += mainwindow.ui

QT += webengine webenginewidgets network websockets sql xml

CONFIG += static
CONFIG += c++14

DEFINES += CRYPTOPP_IMPORTS
DEFINES += QUAZIP_STATIC

unix: QMAKE_CXXFLAGS += -Wno-unused-parameter -Wall -Wextra
#QMAKE_CXXFLAGS += -fsanitize=address
#QMAKE_LFLAGS += -fsanitize=address
QMAKE_LFLAGS += -rdynamic
#QMAKE_CXXFLAGS += -Wall -Werror

#QMAKE_CXXFLAGS += -H

win32: RC_ICONS = ../WalletMetahash.ico
macx: ICON = $${PWD}/../WalletMetahash.icns

unix:!macx: include(../libs-unix.pri)
win32: include(../libs-win.pri)
macx: include(../libs-macos.pri)

win32: QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
win32: QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
win32: QMAKE_CFLAGS -= -Zc:strictStrings
win32: QMAKE_CXXFLAGS -= -Zc:strictStrings

DISTFILES +=

RESOURCES += \
    ../svg1.qrc \
    ../dbupdates/dbupdates.qrc
