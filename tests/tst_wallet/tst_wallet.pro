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
    ../../src/Wallets/Wallet.cpp \
    ../../src/Wallets/EthWallet.cpp \
    ../../src/Wallets/ethtx/scrypt/crypto_scrypt-nosse.cpp \
    ../../src/Wallets/ethtx/scrypt/sha256.cpp \
    ../../src/Wallets/ethtx/cert.cpp \
    ../../src/Wallets/ethtx/rlp.cpp \
    ../../src/Wallets/ethtx/ethtx.cpp \
    ../../src/Wallets/ethtx/cert2.cpp \
    ../../src/Wallets/ethtx/scrypt/crypto_scrypt_saltgen.cpp \
    ../../src/Wallets/ethtx/crossguid/Guid.cpp \
    ../../src/Wallets/btctx/Base58.cpp \
    ../../src/Wallets/btctx/btctx.cpp \
    ../../src/Wallets/btctx/wif.cpp \
    ../../src/Wallets/BtcWallet.cpp \
    ../../src/Wallets/openssl_wrapper/openssl_wrapper.cpp \
    ../../src/utilites/utils.cpp \
    ../../src/Wallets/ethtx/utils2.cpp \
    ../../src/Wallets/WalletInfo.cpp \
    ../LogMock.cpp \
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
