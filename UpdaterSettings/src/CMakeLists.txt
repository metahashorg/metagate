cmake_minimum_required(VERSION 3.1)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${CMAKE_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(Updater WIN32
        main.cpp
        MainWindow.cpp
        MainWindow.hpp
        Settings.cpp
        Settings.hpp
)


target_link_libraries(Updater
        Qt5::Core
        Qt5::Widgets
)
