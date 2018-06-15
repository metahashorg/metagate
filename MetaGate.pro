TEMPLATE = app

DEFINES += VERSION_STRING=\\\"1.13.4\\\"
#DEFINES += DEVELOPMENT
DEFINES += PRODUCTION
DEFINES += APPLICATION_NAME=\\\"MetaGate\\\"

DEFINES += GIT_CURRENT_SHA1="\\\"$$system(git rev-parse --short HEAD)\\\""

unix:INCLUDEPATH = ./src
#unix:INCLUDEPATH = /usr/local/include/c++/7.1/ ./src ./quazip-0.7.3/quazip/
win32:INCLUDEPATH += C:/Qt/5.9.1/msvc2015_64/include/QtZlib ./src ./openssl-1.0.2o-x64/include

INCLUDEPATH += $$PWD/ $$PWD/quazip-0.7.3/
DEPENDPATH += $$PWD/ $$PWD/quazip-0.7.3/

SOURCES += src/main.cpp src/mainwindow.cpp \
    src/Wallet.cpp \
    src/client.cpp \
    src/machine_uid_win.cpp \
    src/unzip.cpp \
    src/uploader.cpp \
    src/EthWallet.cpp \
    src/ethtx/scrypt/crypto_scrypt-nosse.cpp \
    src/ethtx/scrypt/sha256.cpp \
    src/ethtx/cert.cpp \
    src/ethtx/rlp.cpp \
    src/ethtx/ethtx.cpp \
    src/ethtx/cert2.cpp \
    src/ethtx/scrypt/crypto_scrypt_saltgen.cpp \
    src/ethtx/crossguid/Guid.cpp \
    src/btctx/Base58.cpp \
    src/btctx/btctx.cpp \
    src/btctx/wif.cpp \
    src/BtcWallet.cpp \
    src/VersionWrapper.cpp \
    src/StopApplication.cpp \
    src/tests.cpp \
    src/Log.cpp \
    src/openssl_wrapper/openssl_wrapper.cpp \
    src/utils.cpp \
    src/ethtx/utils2.cpp \
    src/tests2.cpp \
    src/NsLookup.cpp \
    src/dns/datatransformer.cpp \
    src/dns/dnspacket.cpp \
    src/dns/resourcerecord.cpp \
    src/WebSocketClient.cpp \
    src/JavascriptWrapper.cpp \
    src/PagesMappings.cpp \
    src/mhurlschemehandler.cpp

unix: SOURCES += src/machine_uid_unix.cpp

HEADERS += src/mainwindow.h \
    src/Wallet.h \
    src/check.h \
    src/machine_uid.h \
    src/client.h \
    src/WindowEvents.h \
    src/unzip.h \
    src/uploader.h \
    src/EthWallet.h \
    src/ethtx/scrypt/libscrypt.h \
    src/ethtx/scrypt/sha256.h \
    src/ethtx/scrypt/sysendian.h \
    src/ethtx/cert.h \
    src/ethtx/const.h \
    src/ethtx/rlp.h \
    src/ethtx/ethtx.h \
    src/ethtx/crossguid/Guid.hpp \
    src/btctx/Base58.h \
    src/btctx/btctx.h \
    src/btctx/wif.h \
    src/btctx/Base58.h \
    src/btctx/btctx.h \
    src/btctx/wif.h \
    src/platform.h \
    src/VersionWrapper.h \
    src/BtcWallet.h \
    src/StopApplication.h \
    src/tests.h \
    src/Log.h \
    src/TypedException.h \
    src/openssl_wrapper/openssl_wrapper.h \
    src/utils.h \
    src/ethtx/utils2.h \
    src/tests2.h \
    src/NsLookup.h \
    src/dns/datatransformer.h \
    src/dns/dnspacket.h \
    src/dns/resourcerecord.h \
    src/WebSocketClient.h \
    src/JavascriptWrapper.h \
    src/algorithms.h \
    src/PagesMappings.h \
    src/SlotWrapper.h \
    src/mhurlschemehandler.h

FORMS += src/mainwindow.ui

QT += webengine webenginewidgets network websockets

CONFIG += static
CONFIG += c++14

DEFINES += CRYPTOPP_IMPORTS
DEFINES += QUAZIP_STATIC

#QMAKE_CXXFLAGS += -fsanitize=address
#QMAKE_LFLAGS += -fsanitize=address
QMAKE_LFLAGS += -rdynamic
#QMAKE_CXXFLAGS += -Wall -Werror

#unix:!macx: INCLUDEPATH += /usr/include /usr/include/x86_64-linux-gnu/ /usr/include/x86_64-linux-gnu/openssl/
unix:!macx: LIBS += -L/usr/lib -lssl -lcrypto
unix:!macx: LIBS += -L$$PWD/cryptopp/lib/linux/ -lcryptopp -L$$PWD/quazip-0.7.3/libs/linux/ -lquazip -lz
unix:!macx: LIBS += -L$$PWD/secp256k1/lib/linux/ -lsecp256k1 -lgmp -luuid

win32: LIBS += -L$$PWD/secp256k1/lib/windows/ -ladvapi32 -lOle32 -llibsecp256k1
win32: LIBS += -L$$PWD/cryptopp/lib/windows/ -lcryptopp -lcryptlib -L$$PWD/quazip-0.7.3/libs/win/ -lquazip
win32: LIBS += -L$$PWD/openssl-1.0.2o-x64/lib/ -llibeay32 -lssleay32 -lws2_32 -lshell32 -ladvapi32 -lgdi32 -lUser32 -lIphlpapi

win32: DEFINES += TARGET_WINDOWS

macx: QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../,-rpath,@executable_path/../,-rpath,@executable_path/../Frameworks

macx: DEFINES += TARGET_OS_MAC

macx: INCLUDEPATH += /usr/local/opt/openssl/include
macx: LIBS += -L$$PWD/cryptopp/lib/mac/ -lcryptopp -L$$PWD/quazip-0.7.3/libs/mac/ -lquazip -lz
macx: LIBS += -L$$PWD/secp256k1/lib/macx/ -lsecp256k1
macx: PRE_TARGETDEPS += $$PWD/cryptopp/lib/mac/libcryptopp.a $$PWD/quazip-0.7.3/libs/mac/libquazip.a
macx: LIBS += /usr/local/opt/openssl/lib/libssl.a /usr/local/opt/openssl/lib/libcrypto.a

macx: ICON = $${PWD}/WalletMetahash.icns

DISTFILES +=

RESOURCES += \
    svg1.qrc
