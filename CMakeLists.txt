cmake_minimum_required(VERSION 3.7)

project(stompconn VERSION 1.5.0 LANGUAGES CXX)

option(STOMPCONN_DEBUG "show debug" OFF)
option(STOMPCONN_LIBRARY_STATIC "Set library type to STATIC" ON)
option(STOMPCONN_LIBBEVENT "add libevent" ON)

macro(use_cxx17)
  if (CMAKE_VERSION VERSION_LESS "3.8.2")
    message(STATUS "stompconn enable c++17")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  else ()
    set (CMAKE_CXX_STANDARD 17)
    set (CMAKE_CXX_STANDARD_REQUIRED ON)
  endif ()
endmacro(use_cxx17)

use_cxx17()

add_definitions("-DSTOMPCONN_VERSION=${PROJECT_VERSION}")

if (WIN32)
    add_definitions("-DNOMINMAX")
endif()

if (STOMPCONN_LIBBEVENT)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libevent/include)
endif()

set(source
  src/frame.cpp
  src/handler.cpp
  src/stomplay.cpp
  src/connection.cpp
  src/version.cpp
  src/libevent.cpp
)

set(pub_header
    include/stompconn/handler.hpp
    include/stompconn/packet.hpp
    include/stompconn/stomplay.hpp
    include/stompconn/frame.hpp
    include/stompconn/connection.hpp
    include/stompconn/version.hpp
    include/stompconn/libevent.hpp
)

if (STOMPCONN_LIBRARY_STATIC)
    add_library(stompconn STATIC ${source})
else()
    set_target_properties(stompconn PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
    add_library(stompconn SHARED ${source})
endif()

target_include_directories(stompconn PUBLIC include)
