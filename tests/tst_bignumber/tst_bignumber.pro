QT       += testlib
QT       -= gui
QT += widgets
TARGET = tst_bignumber
CONFIG   += testcase
CONFIG += c++14
CONFIG += static

TEMPLATE = app

INCLUDEPATH = ../../src

SOURCES += \
    tst_bignumber.cpp \
    ../../src/utilites/BigNumber.cpp

HEADERS += \
    tst_bignumber.h \
    ../../src/utilites/BigNumber.h

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
