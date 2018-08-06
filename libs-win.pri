INCLUDEPATH += C:/Qt/5.10.1/msvc2015_64/include/QtZlib $$PWD/openssl-1.0.2o-x64/include
INCLUDEPATH += $$PWD/quazip-0.7.3/ $$PWD
DEPENDPATH += C:/Qt/5.10.1/msvc2015_64/include/QtZlib $$PWD/openssl-1.0.2o-x64/include
DEFINES += TARGET_WINDOWS
#QR coder libs
INCLUDEPATH += $$PWD/3rdparty/QrCode/include/ $$PWD/3rdparty/ZBar/include/
contains(QT_ARCH, i386) {
    LIBS += -L$$PWD/secp256k1/lib/windows32/ -ladvapi32 -lOle32 -llibsecp256k1
    LIBS += -L$$PWD/cryptopp/lib/windows32/ -lcryptopp -lcryptlib -L$$PWD/quazip-0.7.3/libs/win32/ -lquazip
    LIBS += -L$$PWD/openssl-1.0.2o-x64/lib32/ -llibeay32 -lssleay32 -lws2_32 -lshell32 -ladvapi32 -lgdi32 -lUser32 -lIphlpapi
    #QR coder libs
    LIBS += -L$$PWD/3rdparty/QrCode/vs2015_32/ -L$$PWD/3rdparty/ZBar/vs2015_32/ -lQrCode -lzbar -lwinmm
} else {
    LIBS += -L$$PWD/secp256k1/lib/windows/ -ladvapi32 -lOle32 -llibsecp256k1
    LIBS += -L$$PWD/cryptopp/lib/windows/ -lcryptopp -lcryptlib -L$$PWD/quazip-0.7.3/libs/win/ -lquazip
    LIBS += -L$$PWD/openssl-1.0.2o-x64/lib/ -llibeay32 -lssleay32 -lws2_32 -lshell32 -ladvapi32 -lgdi32 -lUser32 -lIphlpapi
    #QR coder libs
    LIBS += -L$$PWD/3rdparty/QrCode/vs2015_64/ -L$$PWD/3rdparty/ZBar/vs2015_64/ -lQrCode -lzbar -lwinmm
}
