cmake_minimum_required(VERSION 3.10)

project(stompconn VERSION 1.2.0 DESCRIPTION "stomp connection helper")

option(STOMPCONN_DEBUG "show debug" OFF)
option(STOMPCONN_LIBRARY_STATIC "Set library type to STATIC" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-DSTOMPCONN_VERSION=${PROJECT_VERSION}")

set(source
    src/frame.cpp
    src/handler.cpp
    src/stomplay.cpp
    src/connection.cpp
)

set(pub_header
    include/stompconn/handler.hpp
    include/stompconn/packet.hpp
    include/stompconn/stomplay.hpp
    include/stompconn/frame.hpp
    include/stompconn/connection.hpp
    include/stomptalk/header_store.hpp
    include/stomptalk/version.hpp
)

if (STOMPCONN_LIBRARY_STATIC)
    add_library(stompconn STATIC ${source})
else()
    set_target_properties(stompconn PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
    add_library(stompconn SHARED ${source})
endif()

target_include_directories(stompconn PUBLIC include)
