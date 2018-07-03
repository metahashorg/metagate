INCLUDEPATH += /usr/local/include/c++/7.1/
INCLUDEPATH += $$PWD/quazip-0.7.3/ $$PWD
unix:!macx: INCLUDEPATH += $$PWD/openssl_linux/include/
unix:!macx: LIBS += -L$$PWD/openssl_linux/lib -lssl -lcrypto
unix:!macx: LIBS += -L$$PWD/cryptopp/lib/linux/ -lcryptopp -L$$PWD/quazip-0.7.3/libs/linux/ -lquazip -lz
unix:!macx: LIBS += -L$$PWD/secp256k1/lib/linux/ -lsecp256k1 -lgmp -luuid
