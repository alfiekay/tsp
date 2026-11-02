cmake_minimum_required(VERSION 4.3)
project(tsp)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(tsp
    main.cpp
    fns.cpp
    algorithms.cpp
    header.hpp
)
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.7.2/macos")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)

target_include_directories(tsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tsp PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Charts
)
