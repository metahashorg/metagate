TARGET   = httpservice
TEMPLATE = app
CONFIG   += console qt
QT = core network 
CONFIG += c++14
INCLUDEPATH = ./ ./proxy
INCLUDEPATH = ../src
QMAKE_CFLAGS += -std=c99 -Wunused-parameter
SOURCES  = main.cpp

SOURCES +=  ../src/proxy/http_parser.c

SOURCES += \
        \
#        ../src/Log.cpp \
#        ../src/utils.cpp \
#        ../src/Paths.cpp \
#        ../src/btctx/Base58.cpp \
        \
        ../src/proxy/ProxyServer.cpp \
        ../src/proxy/ProxyClient.cpp

HEADERS += \
#        ../src/Log.h \
#        ../src/utils.h \
#        ../src/Paths.h \
#        ../src/btctx/Base58.h \
        \
        ../src/proxy/ProxyServer.h \
        ../src/proxy/ProxyClient.h

include(src/qtservice.pri)
