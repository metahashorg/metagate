INCLUDEPATH += C:/Qt/5.10.1/msvc2015_64/include/QtZlib $$PWD/3rdparty/openssl-1.0.2o-x64/include
INCLUDEPATH += $$PWD/3rdparty $$PWD/3rdparty/quazip/include $$PWD
DEPENDPATH += C:/Qt/5.10.1/msvc2015_64/include/QtZlib $$PWD/openssl-1.0.2o-x64/include
DEFINES += TARGET_WINDOWS
INCLUDEPATH += $$PWD/3rdparty/cryptopp/include/
#QR coder libs
INCLUDEPATH += $$PWD/3rdparty/QrCode/include/ $$PWD/3rdparty/ZBar/include/
contains(QT_ARCH, i386) {
    LIBS += -L$$PWD/3rdparty/secp256k1/windows32/ -ladvapi32 -lOle32 -llibsecp256k1
    LIBS += -L$$PWD/3rdparty/cryptopp/windows32/ -lcryptopp -lcryptlib -L$$PWD/3rdparty/quazip/win32/ -lquazip
    LIBS += -L$$PWD/3rdparty/openssl-1.0.2o-x64/lib32/ -llibeay32 -lssleay32 -lws2_32 -lshell32 -ladvapi32 -lgdi32 -lUser32 -lIphlpapi
    #QR coder libs
    LIBS += -L$$PWD/3rdparty/QrCode/vs2015_32/ -L$$PWD/3rdparty/ZBar/vs2015_32/ -lQrCode -lzbar -lwinmm
} else {
    LIBS += -L$$PWD/3rdparty/secp256k1/windows/ -ladvapi32 -lOle32 -llibsecp256k1
    LIBS += -L$$PWD/3rdparty/cryptopp/windows/ -lcryptopp -lcryptlib -L$$PWD/3rdparty/quazip/win/ -lquazip
    LIBS += -L$$PWD/3rdparty/openssl-1.0.2o-x64/lib/ -llibeay32 -lssleay32 -lws2_32 -lshell32 -ladvapi32 -lgdi32 -lUser32 -lIphlpapi
    #QR coder libs
    LIBS += -L$$PWD/3rdparty/QrCode/vs2015_64/ -L$$PWD/3rdparty/ZBar/vs2015_64/ -lQrCode -lzbar -lwinmm
}
