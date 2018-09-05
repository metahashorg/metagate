QT       += testlib
QT       -= gui
QT += widgets
TARGET = tst_wallet
CONFIG   += testcase
CONFIG += c++14
CONFIG += static

TEMPLATE = app

INCLUDEPATH = ../../src

SOURCES += \
    ../../src/Wallet.cpp \
    ../../src/machine_uid_win.cpp \
    ../../src/EthWallet.cpp \
    ../../src/ethtx/scrypt/crypto_scrypt-nosse.cpp \
    ../../src/ethtx/scrypt/sha256.cpp \
    ../../src/ethtx/cert.cpp \
    ../../src/ethtx/rlp.cpp \
    ../../src/ethtx/ethtx.cpp \
    ../../src/ethtx/cert2.cpp \
    ../../src/ethtx/scrypt/crypto_scrypt_saltgen.cpp \
    ../../src/ethtx/crossguid/Guid.cpp \
    ../../src/btctx/Base58.cpp \
    ../../src/btctx/btctx.cpp \
    ../../src/btctx/wif.cpp \
    ../../src/BtcWallet.cpp \
    ../../src/VersionWrapper.cpp \
    ../../src/openssl_wrapper/openssl_wrapper.cpp \
    ../../src/utils.cpp \
    ../../src/ethtx/utils2.cpp \
    ../../src/Log.cpp \
    ../../src/Paths.cpp \
    tst_Metahash.cpp \
    tst_Bitcoin.cpp \
    tst_Ethereum.cpp \
    tst_rsa.cpp \
    tst_main.cpp

HEADERS += \
    tst_Metahash.h \
    tst_Bitcoin.h \
    tst_Ethereum.h \
    tst_rsa.h

DEFINES += CRYPTOPP_IMPORTS
DEFINES += QUAZIP_STATIC

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
