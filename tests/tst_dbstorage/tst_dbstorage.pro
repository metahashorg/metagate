QT      += testlib
QT      -= gui
QT      += widgets sql
TARGET = tst_dbstorage
CONFIG   += testcase
CONFIG += c++14
CONFIG += static

TEMPLATE = app

INCLUDEPATH = ../../src ../../src/Messenger

SOURCES += \
    tst_messengerdbstorage.cpp \
    ../../src/dbstorage.cpp \
    ../../src/Messenger/messengerdbstorage.cpp


HEADERS += \
    tst_messengerdbstorage.h \
    ../../src/dbstorage.h \
    ../../src/Messenger/messengerdbstorage.h

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
