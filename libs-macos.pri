QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../,-rpath,@executable_path/../,-rpath,@executable_path/../Frameworks
DEFINES += TARGET_OS_MAC
INCLUDEPATH += $$PWD/3rdparty/quazip/include/ $$PWD/3rdparty
INCLUDEPATH += /usr/local/opt/openssl/include
INCLUDEPATH += $$PWD/3rdparty/cryptopp/include
LIBS += -L$$PWD/3rdparty/cryptopp/mac/ -lcryptopp -L$$PWD/3rdparty/quazip/mac/ -lquazip -lz
LIBS += -L$$PWD/3rdparty/secp256k1/macx/ -lsecp256k1
PRE_TARGETDEPS += $$PWD/3rdparty/cryptopp/mac/libcryptopp.a $$PWD/3rdparty/quazip/mac/libquazip.a
LIBS += /usr/local/opt/openssl/lib/libssl.a /usr/local/opt/openssl/lib/libcrypto.a
DEPENDPATH += $$PWD/3rdparty/quazip/include /usr/local/opt/openssl/include
#QR coder libs
INCLUDEPATH += $$PWD/3rdparty/QrCode/include/ $$PWD/3rdparty/ZBar/include/
LIBS += -L$$PWD/3rdparty/QrCode/macos/ -L$$PWD/3rdparty/ZBar/macos/ -lQrCode -lzbar -liconv
QMAKE_LFLAGS += -F/System/Library/Frameworks -L/usr/lib
LIBS += -framework DiskArbitration -framework CoreFoundation
