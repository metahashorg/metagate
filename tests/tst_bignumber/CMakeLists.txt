include_directories(.)

set(TARGET tst_bignumber)

find_package(Qt5Core REQUIRED)
find_package(Qt5Test REQUIRED)

set(${TARGET}_SRCS
    tst_bignumber.cpp
    ../../src/BigNumber.cpp
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
)
