QT      += testlib
QT      -= gui
QT      += widgets sql
TARGET = tst_transactionsdbstorage
CONFIG   += testcase
CONFIG += c++14
CONFIG += static

TEMPLATE = app

INCLUDEPATH = ../../src ../../src/transactions

SOURCES += \
    tst_transactionsdbstorage.cpp \
    ../../src/dbstorage.cpp \
    ../../src/utilites/BigNumber.cpp \
    ../LogMock.cpp \
    ../../src/transactions/TransactionsDBStorage.cpp


HEADERS += \
    tst_transactionsdbstorage.h \
    ../../src/dbstorage.h \
    ../../src/utilites/BigNumber.h \
    ../../src/Log.h \
    ../../src/transactions/TransactionsDBStorage.h

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
