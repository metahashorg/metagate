QT      += testlib
QT      -= gui
QT      += widgets sql
TARGET = tst_walletnamesdbstorage
CONFIG   += testcase
CONFIG += c++14
CONFIG += static

TEMPLATE = app

INCLUDEPATH = ../../src ../../src/WalletNames

SOURCES += \
    tst_walletnamesdbstorage.cpp \
    ../../src/dbstorage.cpp \
    ../../src/BigNumber.cpp \
    ../../src/Log.cpp \
    ../../src/utils.cpp \
    ../../src/Paths.cpp \
    ../../src/merge_settings.cpp \
    ../../src/VersionWrapper.cpp \
    ../../src/btctx/Base58.cpp \
    ../../src/WalletNames/WalletNamesDbStorage.cpp


HEADERS += \
    tst_walletnamesdbstorage.h \
    ../../src/dbstorage.h \
    ../../src/BigNumber.h \
    ../../src/Log.h \
    ../../src/WalletNames/WalletNamesDbStorage.h

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
