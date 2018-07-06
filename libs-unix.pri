INCLUDEPATH += /usr/local/include/c++/7.1/
INCLUDEPATH += $$PWD/quazip-0.7.3/ $$PWD
INCLUDEPATH += $$PWD/openssl_linux/include/
LIBS += -L$$PWD/openssl_linux/lib -lssl -lcrypto
LIBS += -L$$PWD/cryptopp/lib/linux/ -lcryptopp -L$$PWD/quazip-0.7.3/libs/linux/ -lquazip -lz
LIBS += -L$$PWD/secp256k1/lib/linux/ -lsecp256k1 -lgmp -luuid
