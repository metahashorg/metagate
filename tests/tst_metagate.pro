QT       += testlib
QT       -= gui
QT += widgets
TARGET = tst_metagate
CONFIG   += testcase

TEMPLATE = app

INCLUDEPATH = ../ ../src
#INCLUDEPATH += ../../lib
#LIBS += -L../../lib/bin -lFountain

SOURCES += \
    tst_metagate.cpp \
    ../src/Wallet.cpp \
    ../src/machine_uid_win.cpp \
    ../src/EthWallet.cpp \
    ../src/ethtx/scrypt/crypto_scrypt-nosse.cpp \
    ../src/ethtx/scrypt/sha256.cpp \
    ../src/ethtx/cert.cpp \
    ../src/ethtx/rlp.cpp \
    ../src/ethtx/ethtx.cpp \
    ../src/ethtx/cert2.cpp \
    ../src/ethtx/scrypt/crypto_scrypt_saltgen.cpp \
    ../src/ethtx/crossguid/Guid.cpp \
    ../src/btctx/Base58.cpp \
    ../src/btctx/btctx.cpp \
    ../src/btctx/wif.cpp \
    ../src/BtcWallet.cpp \
    ../src/VersionWrapper.cpp \
    ../src/openssl_wrapper/openssl_wrapper.cpp \
    ../src/utils.cpp \
    ../src/ethtx/utils2.cpp \
    ../src/Log.cpp

HEADERS += \
    tst_metagate.h


unix:!macx: INCLUDEPATH += $$PWD/../openssl_linux/include/
unix:!macx: LIBS += -L$$PWD/../openssl_linux/lib -lssl -lcrypto
unix:!macx: LIBS += -L$$PWD/../cryptopp/lib/linux/ -lcryptopp -L$$PWD/../quazip-0.7.3/libs/linux/ -lquazip -lz
unix:!macx: LIBS += -L$$PWD/../secp256k1/lib/linux/ -lsecp256k1 -lgmp -luuid


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
