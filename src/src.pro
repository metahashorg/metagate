TEMPLATE = app

TARGET = MetaGate

DEFINES += VERSION_STRING=\\\"1.19.7\\\"
DEFINES += VERSION_SETTINGS=\\\"10.4\\\"
#DEFINES += DEVELOPMENT
DEFINES += PRODUCTION
DEFINES += APPLICATION_NAME=\\\"MetaGate\\\"

DEFINES += GIT_CURRENT_SHA1="\\\"$$system(git rev-parse --short HEAD)\\\""

QMAKE_INFO_PLIST +=  ../deploy/mac/default.plist

SOURCES += main.cpp mainwindow.cpp \
    Wallet.cpp \
    uploader.cpp \
    EthWallet.cpp \
    ethtx/scrypt/crypto_scrypt-nosse.cpp \
    ethtx/scrypt/sha256.cpp \
    ethtx/cert.cpp \
    ethtx/rlp.cpp \
    ethtx/ethtx.cpp \
    ethtx/cert2.cpp \
    ethtx/scrypt/crypto_scrypt_saltgen.cpp \
    ethtx/crossguid/Guid.cpp \
    btctx/Base58.cpp \
    btctx/btctx.cpp \
    btctx/wif.cpp \
    BtcWallet.cpp \
    StopApplication.cpp \
    tests.cpp \
    openssl_wrapper/openssl_wrapper.cpp \
    ethtx/utils2.cpp \
    NsLookup/NsLookup.cpp \
    dns/datatransformer.cpp \
    dns/dnspacket.cpp \
    dns/resourcerecord.cpp \
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
    WalletRsa.cpp \
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
    Network/WebSocketClient.cpp

unix: SOURCES +=
SOURCES +=  proxy/http_parser.c

HEADERS += mainwindow.h \
    Wallet.h \
    uploader.h \
    EthWallet.h \
    ethtx/scrypt/libscrypt.h \
    ethtx/scrypt/sha256.h \
    ethtx/scrypt/sysendian.h \
    ethtx/cert.h \
    ethtx/const.h \
    ethtx/rlp.h \
    ethtx/ethtx.h \
    ethtx/crossguid/Guid.hpp \
    btctx/Base58.h \
    btctx/btctx.h \
    btctx/wif.h \
    btctx/Base58.h \
    btctx/btctx.h \
    btctx/wif.h \
    BtcWallet.h \
    StopApplication.h \
    tests.h \
    openssl_wrapper/openssl_wrapper.h \
    ethtx/utils2.h \
    NsLookup/NsLookup.h \
    dns/datatransformer.h \
    dns/dnspacket.h \
    dns/resourcerecord.h \
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
    WalletRsa.h \
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
    Network/WebSocketClient.h

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
