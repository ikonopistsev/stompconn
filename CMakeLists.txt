cmake_minimum_required(VERSION 3.14)

project(stompconn VERSION 1.0.0 DESCRIPTION "stomp connection helper")

option(STOMPCONN_DEBUG "show debug" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

add_library(stompconn STATIC ${source})

target_include_directories(stompconn PUBLIC include)

include(GNUInstallDirs)

set_target_properties(stompconn PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

install(TARGETS stompconn
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${pub_header} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/stompconn)

