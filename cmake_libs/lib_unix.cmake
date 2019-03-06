function(add_common_options_internal TARGET CURR_PATH)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../quazip-0.7.3/)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../openssl_linux/include/)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../3rdparty/QrCode/include/)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../3rdparty/ZBar/include/)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../secp256k1/include/)

    find_library(SSL_LIBRARY NAMES ssl HINTS ${CURR_PATH}/../openssl_linux/lib)
    find_library(CRYPTO_LIBRARY NAMES crypto HINTS ${CURR_PATH}/../openssl_linux/lib)
    find_library(CRYPTOPP_LIBRARY NAMES cryptopp HINTS ${CURR_PATH}/../cryptopp/lib/linux/)
    find_library(QUAZIP_LIBRARY NAMES quazip HINTS ${CURR_PATH}/../quazip-0.7.3/libs/linux/)
    find_library(SECP_LIBRARY NAMES secp256k1 HINTS ${CURR_PATH}/../secp256k1/lib/linux/)
    find_library(QR_CODE_LIBRARY NAMES QrCode HINTS ${CURR_PATH}/../3rdparty/QrCode/linux/)
    find_library(ZBAR_LIBRARY NAMES zbar HINTS ${CURR_PATH}/../3rdparty/ZBar/linux/)

    find_package(Threads)

    target_link_libraries(${TARGET} 
        ${CMAKE_THREAD_LIBS_INIT}
        ${SSL_LIBRARY}
        ${CRYPTO_LIBRARY}
        ${CRYPTOPP_LIBRARY}
        ${QUAZIP_LIBRARY}
        z
        ${SECP_LIBRARY}
        gmp
        uuid
        ${QR_CODE_LIBRARY}
        ${ZBAR_LIBRARY}
    )

    #ubuntu18 flags
    target_compile_definitions(${TARGET} PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
    target_compile_options(${TARGET} PUBLIC -no-pie)
endfunction(add_common_options_internal)
