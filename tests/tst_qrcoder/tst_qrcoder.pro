QT       += testlib
QT       -= gui
QT += widgets
TARGET = tst_qrcoder
CONFIG   += testcase
CONFIG += c++14
CONFIG += static

TEMPLATE = app

INCLUDEPATH = ../../src

SOURCES += \
    tst_qrcoder.cpp \
    ../../src/qrcoder.cpp

HEADERS += \
    tst_qrcoder.h

QMAKE_LFLAGS += -rdynamic
unix:!macx: include(../../libs-unix.pri)
win32: include(../../libs-win.pri)
macx: include(../../libs-macos.pri)
