win32:INCLUDEPATH += C:/Qt/5.10.1/msvc2015_64/include/QtZlib $$PWD/openssl-1.0.2o-x64/include
INCLUDEPATH += $$PWD/quazip-0.7.3/ $$PWD
win32:DEPENDPATH += C:/Qt/5.10.1/msvc2015_64/include/QtZlib $$PWD/openssl-1.0.2o-x64/include
win32: LIBS += -L$$PWD/secp256k1/lib/windows/ -ladvapi32 -lOle32 -llibsecp256k1
win32: LIBS += -L$$PWD/cryptopp/lib/windows/ -lcryptopp -lcryptlib -L$$PWD/quazip-0.7.3/libs/win/ -lquazip
win32: LIBS += -L$$PWD/openssl-1.0.2o-x64/lib/ -llibeay32 -lssleay32 -lws2_32 -lshell32 -ladvapi32 -lgdi32 -lUser32 -lIphlpapi
win32: DEFINES += TARGET_WINDOWS
