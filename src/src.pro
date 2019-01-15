TEMPLATE = app

TARGET = MetaGate

DEFINES += VERSION_STRING=\\\"1.18.0\\\"
DEFINES += VERSION_SETTINGS=\\\"5\\\"
#DEFINES += DEVELOPMENT
DEFINES += PRODUCTION
DEFINES += APPLICATION_NAME=\\\"MetaGate\\\"

DEFINES += GIT_CURRENT_SHA1="\\\"$$system(git rev-parse --short HEAD)\\\""

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
    Module.cpp \
    Messenger/CryptographicManager.cpp

unix: SOURCES += machine_uid_unix.cpp
SOURCES +=  proxy/http_parser.c

HEADERS += mainwindow.h \
    Wallet.h \
    check.h \
    machine_uid.h \
    client.h \
    WindowEvents.h \
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
    Module.h \
    Messenger/CryptographicManager.h \
    CallbackWrapper.h

FORMS += mainwindow.ui

QT += webengine webenginewidgets network websockets sql xml

CONFIG += static
CONFIG += c++14

DEFINES += CRYPTOPP_IMPORTS
DEFINES += QUAZIP_STATIC

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

DISTFILES +=

RESOURCES += \
    ../svg1.qrc \
    ../dbupdates/dbupdates.qrc
