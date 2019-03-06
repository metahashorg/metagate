include_directories(.)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_libs/")
include(lib_common)

add_definitions(-DVERSION_STRING="1.19.0")
add_definitions(-DVERSION_SETTINGS="10.1")
#add_definitions(-DDEVELOPMENT)
add_definitions(-DPRODUCTION)
add_definitions(-DAPPLICATION_NAME="MetaGate")

add_definitions(-DGIT_CURRENT_SHA1="sha1")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WebEngine REQUIRED)
find_package(Qt5WebEngineWidgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5WebSockets REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Xml REQUIRED)

set(${PROJECT_NAME}_QRC
    ../svg1.qrc
    ../dbupdates/dbupdates.qrc
)

set(${PROJECT_NAME}_UI
    mainwindow.ui
)

set(${PROJECT_NAME}_HEADERS
    mainwindow.h
    Wallet.h
    check.h
    machine_uid.h
    client.h
    unzip.h
    uploader.h
    EthWallet.h
    ethtx/scrypt/libscrypt.h
    ethtx/scrypt/sha256.h
    ethtx/scrypt/sysendian.h
    ethtx/cert.h
    ethtx/const.h
    ethtx/rlp.h
    ethtx/ethtx.h
    ethtx/crossguid/Guid.hpp
    btctx/Base58.h
    btctx/btctx.h
    btctx/wif.h
    btctx/Base58.h
    btctx/btctx.h
    btctx/wif.h
    platform.h
    VersionWrapper.h
    BtcWallet.h
    StopApplication.h
    tests.h
    Log.h
    TypedException.h
    openssl_wrapper/openssl_wrapper.h
    utils.h
    ethtx/utils2.h
    NsLookup.h
    dns/datatransformer.h
    dns/dnspacket.h
    dns/resourcerecord.h
    WebSocketClient.h
    JavascriptWrapper.h
    algorithms.h
    PagesMappings.h
    SlotWrapper.h
    mhurlschemehandler.h
    Paths.h
    BigNumber.h
    RunGuard.h
    makeJsFunc.h
    qrcoder.h
    Messenger/Messenger.h
    TimerClass.h
    Messenger/MessengerMessages.h
    Messenger/MessengerJavascript.h
    Messenger/Message.h
    RequestId.h
    dbstorage.h
    WalletRsa.h
    Messenger/MessengerDBStorage.h
    transactions/Transactions.h
    transactions/TransactionsMessages.h
    transactions/Transaction.h
    transactions/TransactionsDBStorage.h
    transactions/TransactionsJavascript.h
    HttpClient.h
    duration.h
    proxy/UPnPDevices.h
    proxy/UPnPRouter.h
    proxy/ProxyServer.h
    proxy/ProxyClient.h
    proxy/Proxy.h
    proxy/ProxyJavascript.h
    auth/Auth.h
    auth/AuthJavascript.h
    Initializer/Initializer.h
    Initializer/InitializerJavascript.h
    Initializer/InitInterface.h
    Initializer/Inits/InitMainwindow.h
    Initializer/Inits/InitAuth.h
    Initializer/Inits/InitNsLookup.h
    Initializer/Inits/InitTransactions.h
    Initializer/Inits/InitWebSocket.h
    Initializer/Inits/InitJavascriptWrapper.h
    Initializer/Inits/InitUploader.h
    Module.h
    Messenger/CryptographicManager.h
    CallbackWrapper.h
    proxy/WebSocketSender.h
    QRegister.h
    Initializer/Inits/InitProxy.h
    Initializer/Inits/InitMessenger.h
    UdpSocketClient.h
)

set(${PROJECT_NAME}_SRCS
    main.cpp 
    mainwindow.cpp 
    Wallet.cpp
    client.cpp
    machine_uid_win.cpp
    unzip.cpp
    uploader.cpp
    EthWallet.cpp
    ethtx/scrypt/crypto_scrypt-nosse.cpp
    ethtx/scrypt/sha256.cpp
    ethtx/cert.cpp
    ethtx/rlp.cpp
    ethtx/ethtx.cpp
    ethtx/cert2.cpp
    ethtx/scrypt/crypto_scrypt_saltgen.cpp
    ethtx/crossguid/Guid.cpp
    btctx/Base58.cpp
    btctx/btctx.cpp
    btctx/wif.cpp
    BtcWallet.cpp
    VersionWrapper.cpp
    StopApplication.cpp
    tests.cpp
    Log.cpp
    openssl_wrapper/openssl_wrapper.cpp
    utils.cpp
    ethtx/utils2.cpp
    NsLookup.cpp
    dns/datatransformer.cpp
    dns/dnspacket.cpp
    dns/resourcerecord.cpp
    WebSocketClient.cpp
    JavascriptWrapper.cpp
    PagesMappings.cpp
    mhurlschemehandler.cpp
    Paths.cpp
    BigNumber.cpp
    RunGuard.cpp
    qrcoder.cpp
    Messenger/Messenger.cpp
    TimerClass.cpp
    Messenger/MessengerMessages.cpp
    Messenger/MessengerJavascript.cpp
    Messenger/CryptographicManager.cpp
    dbstorage.cpp
    WalletRsa.cpp
    TypedException.cpp
    Messenger/MessengerDBStorage.cpp
    transactions/Transactions.cpp
    transactions/TransactionsMessages.cpp
    transactions/TransactionsDBStorage.cpp
    transactions/TransactionsJavascript.cpp
    HttpClient.cpp
    proxy/UPnPDevices.cpp
    proxy/UPnPRouter.cpp
    proxy/ProxyServer.cpp
    proxy/ProxyClient.cpp
    proxy/Proxy.cpp
    proxy/ProxyJavascript.cpp
    auth/Auth.cpp
    auth/AuthJavascript.cpp
    machine_uid.cpp
    Initializer/Initializer.cpp
    Initializer/InitializerJavascript.cpp
    Initializer/InitInterface.cpp
    Initializer/Inits/InitMainwindow.cpp
    Initializer/Inits/InitAuth.cpp
    Initializer/Inits/InitNsLookup.cpp
    Initializer/Inits/InitTransactions.cpp
    Initializer/Inits/InitWebSocket.cpp
    Initializer/Inits/InitJavascriptWrapper.cpp
    Initializer/Inits/InitUploader.cpp
    Module.cpp
    proxy/WebSocketSender.cpp
    QRegister.cpp
    Initializer/Inits/InitProxy.cpp
    Initializer/Inits/InitMessenger.cpp
    UdpSocketClient.cpp
    proxy/http_parser.c
)

if (UNIX)
    list(APPEND ${PROJECT_NAME}_SRCS machine_uid_unix.cpp)
elseif(WIN32)
    list(APPEND ${PROJECT_NAME}_SRCS machine_uid_win.cpp)
endif()

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_QRC} ${${PROJECT_NAME}_UI} ${${PROJECT_NAME}_HEADERS})

target_compile_definitions(${PROJECT_NAME} PUBLIC -DCRYPTOPP_IMPORTS)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DQUAZIP_STATIC)

target_compile_options(${PROJECT_NAME} PUBLIC -rdynamic)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unused-parameter -Wall -Wextra)
endif()

add_common_options(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::WebEngine
    Qt5::WebEngineWidgets
    Qt5::Network
    Qt5::WebSockets
    Qt5::Sql
    Qt5::Xml
)
