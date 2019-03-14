QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../,-rpath,@executable_path/../,-rpath,@executable_path/../Frameworks
DEFINES += TARGET_OS_MAC
INCLUDEPATH += $$PWD/quazip-0.7.3/ $$PWD
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L$$PWD/cryptopp/lib/mac/ -lcryptopp -L$$PWD/quazip-0.7.3/libs/mac/ -lquazip -lz
LIBS += -L$$PWD/secp256k1/lib/macx/ -lsecp256k1
PRE_TARGETDEPS += $$PWD/cryptopp/lib/mac/libcryptopp.a $$PWD/quazip-0.7.3/libs/mac/libquazip.a
LIBS += /usr/local/opt/openssl/lib/libssl.a /usr/local/opt/openssl/lib/libcrypto.a
DEPENDPATH += $$PWD/quazip-0.7.3/ /usr/local/opt/openssl/include
#QR coder libs
INCLUDEPATH += $$PWD/3rdparty/QrCode/include/ $$PWD/3rdparty/ZBar/include/
LIBS += -L$$PWD/3rdparty/QrCode/macos/ -L$$PWD/3rdparty/ZBar/macos/ -lQrCode -lzbar -liconv
QMAKE_LFLAGS += -F/System/Library/Frameworks -L/usr/lib
LIBS += -framework DiskArbitration -framework CoreFoundation
