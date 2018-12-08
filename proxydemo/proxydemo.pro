#-------------------------------------------------
#
# Project created by QtCreator 2018-11-20T18:49:51
#
#-------------------------------------------------

QT       += core gui widgets network  xml

TARGET = proxydemo
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++14

INCLUDEPATH = ./ ./proxy
INCLUDEPATH = ../src

QMAKE_CFLAGS += -std=c99 -Wunused-parameter
SOURCES +=  ../src/proxy/http_parser.c

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        \
        ../src/Log.cpp \
        ../src/utils.cpp \
        ../src/Paths.cpp \
        ../src/btctx/Base58.cpp \
        \
        ../src/proxy/UPnPDevices.cpp \
        ../src/proxy/UPnPRouter.cpp \
        ../src/proxy/ProxyServer.cpp \
        ../src/proxy/ProxyClient.cpp

HEADERS += \
        mainwindow.h \
        ../src/Log.h \
        ../src/utils.h \
        ../src/Paths.h \
        ../src/btctx/Base58.h \
        \
        ../src/proxy/UPnPDevices.h \
        ../src/proxy/UPnPRouter.h \
        ../src/proxy/ProxyServer.h \
        ../src/proxy/ProxyClient.h

FORMS += \
        mainwindow.ui
