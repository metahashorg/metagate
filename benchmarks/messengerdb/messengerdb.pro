QT -= gui
QT += sql widgets

CONFIG += c++14 console
CONFIG -= app_bundle

INCLUDEPATH = ../../src ../../src/Messenger

SOURCES += \
    main.cpp \
    ../../src/dbstorage.cpp \
    ../../src/Log.cpp \
    ../../src/utils.cpp \
    ../../src/Paths.cpp \
    ../../src/btctx/Base58.cpp \
    ../../src/Messenger/MessengerDBStorage.cpp


HEADERS += \
    ../../src/dbstorage.h \
    ../../src/Log.h \
    ../../src/utils.h \
    ../../src/Paths.h \
    ../../src/btctx/Base58.h \
    ../../src/Messenger/MessengerDBStorage.h
