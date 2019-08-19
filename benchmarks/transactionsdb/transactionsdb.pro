QT -= gui
QT += sql widgets

CONFIG += c++14 console
CONFIG -= app_bundle

INCLUDEPATH = ../../src ../../src/transactions

SOURCES += \
    main.cpp \
    ../../src/dbstorage.cpp \
    ../../src/BigNumber.cpp \
    ../../src/Log.cpp \
    ../../src/utils.cpp \
    ../../src/Paths.cpp \
    ../../src/btctx/Base58.cpp \
    ../../src/transactions/TransactionsDBStorage.cpp


HEADERS += \
    ../../src/dbstorage.h \
    ../../src/BigNumber.h \
    ../../src/Log.h \
    ../../src/utils.h \
    ../../src/Paths.h \
    ../../src/btctx/Base58.h \
    ../../src/transactions/TransactionsDBStorage.h

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
