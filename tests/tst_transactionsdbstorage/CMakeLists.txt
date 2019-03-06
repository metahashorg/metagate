include_directories(.)

set(TARGET tst_transactionsdbstorage)

include(../../cmake_libs/lib_common.cmake)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Sql REQUIRED)

set(${TARGET}_SRCS
    tst_transactionsdbstorage.cpp
    ../../src/dbstorage.cpp
    ../../src/BigNumber.cpp
    ../../src/Log.cpp
    ../../src/utils.cpp
    ../../src/Paths.cpp
    ../../src/btctx/Base58.cpp
    ../../src/transactions/TransactionsDBStorage.cpp
)

add_executable(${TARGET} ${${TARGET}_SRCS})

target_include_directories(${TARGET} PUBLIC ../../src)

target_compile_options(${TARGET} PUBLIC -rdynamic)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${TARGET} PUBLIC -Wno-unused-parameter -Wall -Wextra)
endif()

add_common_options(${TARGET})

target_link_libraries(${TARGET}
    Qt5::Core
    Qt5::Test
    Qt5::Widgets
    Qt5::Sql
)
