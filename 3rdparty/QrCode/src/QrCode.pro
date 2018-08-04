QT       -= core gui

TARGET = QrCode
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
            QrCode.cpp \
	    QrSegment.cpp \
	    BitBuffer.cpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}
