function(add_common_options_internal TARGET)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../quazip-0.7.3/)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../openssl_linux/include/)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/QrCode/include/)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/ZBar/include/)

    find_library(SSL_LIBRARY NAMES ssl HINTS ${CMAKE_CURRENT_LIST_DIR}/../openssl_linux/lib)
    find_library(CRYPTO_LIBRARY NAMES crypto HINTS ${CMAKE_CURRENT_LIST_DIR}/../openssl_linux/lib)
    find_library(CRYPTOPP_LIBRARY NAMES cryptopp HINTS ${CMAKE_CURRENT_LIST_DIR}/../cryptopp/lib/linux/)
    find_library(QUAZIP_LIBRARY NAMES quazip HINTS ${CMAKE_CURRENT_LIST_DIR}/../quazip-0.7.3/libs/linux/)
    find_library(SECP_LIBRARY NAMES secp256k1 HINTS ${CMAKE_CURRENT_LIST_DIR}/../secp256k1/lib/linux/)
    find_library(QR_CODE_LIBRARY NAMES QrCode HINTS ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/QrCode/linux/)
    find_library(ZBAR_LIBRARY NAMES zbar HINTS ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/ZBar/linux/)

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
