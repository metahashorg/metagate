include_directories(.)

include(../cmake_libs/lib_common.cmake)

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

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_QRC} ${${PROJECT_NAME}_UI})

target_compile_definitions(${PROJECT_NAME} PUBLIC -DCRYPTOPP_IMPORTS)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DQUAZIP_STATIC)

target_compile_options(${PROJECT_NAME} PUBLIC -rdynamic)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unused-parameter -Wall -Wextra)
endif()

add_common_options(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::WebEngine
    Qt5::WebEngineWidgets
    Qt5::Network
    Qt5::WebSockets
    Qt5::Sql
    Qt5::Xml
)
