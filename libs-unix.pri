INCLUDEPATH += /usr/local/include/c++/7.1/
INCLUDEPATH += $$PWD/3rdparty $$PWD/3rdparty/quazip/include $$PWD
INCLUDEPATH += $$PWD/openssl_linux/include/
LIBS += -L$$PWD/openssl_linux/lib -lssl -lcrypto
LIBS += -L$$PWD/cryptopp/lib/linux/ -lcryptopp -L$$PWD/3rdparty/quazip/linux/ -lquazip -lz
LIBS += -L$$PWD/3rdparty/secp256k1/linux/ -lsecp256k1 -lgmp -luuid
#QR coder libs
INCLUDEPATH += $$PWD/3rdparty/QrCode/include/ $$PWD/3rdparty/ZBar/include/
LIBS += -L$$PWD/3rdparty/QrCode/linux/ -L$$PWD/3rdparty/ZBar/linux/ -lQrCode -lzbar

QMAKE_LFLAGS += -static-libstdc++ -static-libgcc

#ubuntu18 flags
DEFINES += _GLIBCXX_USE_CXX11_ABI=0
QMAKE_CXXFLAGS += -no-pie
QMAKE_LFLAGS += -no-pie
