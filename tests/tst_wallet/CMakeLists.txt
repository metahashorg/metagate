include_directories(.)

set(TARGET tst_wallet)

include(../../cmake_libs/lib_common.cmake)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Test REQUIRED)

set(${TARGET}_SRCS
    ../../src/Wallet.cpp
    ../../src/EthWallet.cpp
    ../../src/ethtx/scrypt/crypto_scrypt-nosse.cpp
    ../../src/ethtx/scrypt/sha256.cpp
    ../../src/ethtx/cert.cpp
    ../../src/ethtx/rlp.cpp
    ../../src/ethtx/ethtx.cpp
    ../../src/ethtx/cert2.cpp
    ../../src/ethtx/scrypt/crypto_scrypt_saltgen.cpp
    ../../src/ethtx/crossguid/Guid.cpp
    ../../src/btctx/Base58.cpp
    ../../src/btctx/btctx.cpp
    ../../src/btctx/wif.cpp
    ../../src/BtcWallet.cpp
    ../../src/openssl_wrapper/openssl_wrapper.cpp
    ../../src/utils.cpp
    ../../src/ethtx/utils2.cpp
    ../../src/Log.cpp
    ../../src/Paths.cpp
    tst_Metahash.cpp
    tst_Bitcoin.cpp
    tst_Ethereum.cpp
    tst_rsa.cpp
    tst_main.cpp
)

add_executable(${TARGET} ${${TARGET}_SRCS})

target_include_directories(${TARGET} PUBLIC ../../src)

target_compile_definitions(${TARGET} PUBLIC -DCRYPTOPP_IMPORTS)
target_compile_definitions(${TARGET} PUBLIC -DQUAZIP_STATIC)

target_compile_options(${TARGET} PUBLIC -rdynamic)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${TARGET} PUBLIC -Wno-unused-parameter -Wall -Wextra)
endif()

add_common_options(${TARGET})

target_link_libraries(${TARGET}
    Qt5::Core
    Qt5::Test
    Qt5::Widgets
)
