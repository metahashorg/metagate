function(add_common_options_internal TARGET CURR_PATH)
    target_compile_options(${TARGET} PUBLIC "-Wl,-rpath,@loader_path/../,-rpath,@executable_path/../,-rpath,@executable_path/../Frameworks")
    target_compile_definitions(${TARGET} PUBLIC TARGET_OS_MAC)

    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../3rdparty/quazip-0.7.3/)
    target_include_directories(${TARGET} PUBLIC /usr/local/opt/openssl/include)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../3rdparty/QrCode/include/)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../3rdparty/ZBar/include/)
    target_include_directories(${TARGET} PUBLIC ${CURR_PATH}/../3rdparty/secp256k1/include/)

    find_library(SSL_LIBRARY NAMES libssl.a HINTS /usr/local/opt/openssl/lib/)
    find_library(CRYPTO_LIBRARY NAMES libcrypto.a HINTS /usr/local/opt/openssl/lib/)
    find_library(CRYPTOPP_LIBRARY NAMES libcryptopp.a HINTS ${CURR_PATH}/../3rdparty/cryptopp/lib/mac/)
    find_library(QUAZIP_LIBRARY NAMES libquazip.a HINTS ${CURR_PATH}/../3rdparty/quazip-0.7.3/libs/mac/)
    find_library(SECP_LIBRARY NAMES secp256k1 HINTS ${CURR_PATH}/../3rdparty/secp256k1/lib/mac/)
    find_library(QR_CODE_LIBRARY NAMES QrCode HINTS ${CURR_PATH}/../3rdparty/QrCode/macos/)
    find_library(ZBAR_LIBRARY NAMES zbar HINTS ${CURR_PATH}/../3rdparty/ZBar/macos/)

    find_package(Threads)

    target_link_libraries(${TARGET} 
        ${SSL_LIBRARY}
        ${CRYPTO_LIBRARY}
        ${CRYPTOPP_LIBRARY}
        ${QUAZIP_LIBRARY}
        z
        ${SECP_LIBRARY}
        uuid
        iconv
        ${QR_CODE_LIBRARY}
        ${ZBAR_LIBRARY}
    )
endfunction(add_common_options_internal)
