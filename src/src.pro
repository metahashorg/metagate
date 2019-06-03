TEMPLATE = app

TARGET = MetaGate

DEFINES += VERSION_STRING=\\\"1.19.5\\\"
DEFINES += VERSION_SETTINGS=\\\"10.4\\\"
#DEFINES += DEVELOPMENT
DEFINES += PRODUCTION
DEFINES += APPLICATION_NAME=\\\"MetaGate\\\"

DEFINES += GIT_CURRENT_SHA1="\\\"$$system(git rev-parse --short HEAD)\\\""

QMAKE_INFO_PLIST +=  ../deploy/mac/default.plist

SOURCES += main.cpp mainwindow.cpp \
    Wallet.cpp \
    client.cpp \
    machine_uid_win.cpp \
    unzip.cpp \
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
    VersionWrapper.cpp \
    StopApplication.cpp \
    tests.cpp \
    Log.cpp \
    openssl_wrapper/openssl_wrapper.cpp \
    utils.cpp \
    ethtx/utils2.cpp \
    NsLookup.cpp \
    dns/datatransformer.cpp \
    dns/dnspacket.cpp \
    dns/resourcerecord.cpp \
    WebSocketClient.cpp \
    JavascriptWrapper.cpp \
    PagesMappings.cpp \
    mhurlschemehandler.cpp \
    Paths.cpp \
    BigNumber.cpp \
    RunGuard.cpp \
    qrcoder.cpp \
    Messenger/Messenger.cpp \
    TimerClass.cpp \
    Messenger/MessengerMessages.cpp \
    Messenger/MessengerJavascript.cpp \
    Messenger/CryptographicManager.cpp \
    dbstorage.cpp \
    WalletRsa.cpp \
    TypedException.cpp \
    Messenger/MessengerDBStorage.cpp \
    transactions/Transactions.cpp \
    transactions/TransactionsMessages.cpp \
    transactions/TransactionsDBStorage.cpp \
    transactions/TransactionsJavascript.cpp \
    HttpClient.cpp \
    proxy/UPnPDevices.cpp \
    proxy/UPnPRouter.cpp \
    proxy/ProxyServer.cpp \
    proxy/ProxyClient.cpp \
    proxy/Proxy.cpp \
    proxy/ProxyJavascript.cpp \
    auth/Auth.cpp \
    auth/AuthJavascript.cpp \
    machine_uid.cpp \
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
    QRegister.cpp \
    Initializer/Inits/InitProxy.cpp \
    Initializer/Inits/InitMessenger.cpp \
    UdpSocketClient.cpp \
    MhPayEventHandler.cpp \
    WalletNames/WalletNamesDbStorage.cpp \
    WalletNames/WalletNames.cpp \
    WalletNames/WalletNamesJavascript.cpp \
    Initializer/Inits/InitWalletsNames.cpp \
    NetwrokTesting.cpp \
    WalletNames/WalletNamesMessages.cpp

unix: SOURCES += machine_uid_unix.cpp
SOURCES +=  proxy/http_parser.c

HEADERS += mainwindow.h \
    Wallet.h \
    check.h \
    machine_uid.h \
    client.h \
    unzip.h \
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
    platform.h \
    VersionWrapper.h \
    BtcWallet.h \
    StopApplication.h \
    tests.h \
    Log.h \
    TypedException.h \
    openssl_wrapper/openssl_wrapper.h \
    utils.h \
    ethtx/utils2.h \
    NsLookup.h \
    dns/datatransformer.h \
    dns/dnspacket.h \
    dns/resourcerecord.h \
    WebSocketClient.h \
    JavascriptWrapper.h \
    algorithms.h \
    PagesMappings.h \
    SlotWrapper.h \
    mhurlschemehandler.h \
    Paths.h \
    BigNumber.h \
    RunGuard.h \
    makeJsFunc.h \
    qrcoder.h \
    Messenger/Messenger.h \
    TimerClass.h \
    Messenger/MessengerMessages.h \
    Messenger/MessengerJavascript.h \
    Messenger/Message.h \
    RequestId.h \
    dbstorage.h \
    WalletRsa.h \
    Messenger/MessengerDBStorage.h \
    transactions/Transactions.h \
    transactions/TransactionsMessages.h \
    transactions/Transaction.h \
    transactions/TransactionsDBStorage.h \
    transactions/TransactionsJavascript.h \
    HttpClient.h \
    duration.h \
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
    CallbackWrapper.h \
    proxy/WebSocketSender.h \
    QRegister.h \
    Initializer/Inits/InitProxy.h \
    Initializer/Inits/InitMessenger.h \
    UdpSocketClient.h \
    MhPayEventHandler.h \
    WalletNames/WalletNamesDbStorage.h \
    WalletNames/WalletNamesDbRes.h \
    WalletNames/WalletInfo.h \
    WalletNames/WalletNames.h \
    WalletNames/WalletNamesJavascript.h \
    Initializer/Inits/InitWalletsNames.h \
    NetwrokTesting.h \
    WalletNames/WalletNamesMessages.h

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
