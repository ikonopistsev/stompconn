cmake_minimum_required(VERSION 3.10)

project(stompconn VERSION 1.6.5 LANGUAGES CXX)

option(STOMPCONN_DEBUG "show debug" OFF)
option(STOMPCONN_LIBRARY_STATIC "Set library type to STATIC" ON)
option(STOMPCONN_WITH_STATIC_LIBEVENT "build with static libevent" OFF)
option(STOMPCONN_OPENSSL "enable ssl" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-DSTOMPCONN_VERSION=${PROJECT_VERSION}")

if (STOMPCONN_DEBUG)
  add_definitions("-DSTOMPCONN_DEBUG")
endif()

if (STOMPCONN_OPENSSL)
    add_definitions("-DSTOMPCONN_OPENSSL")
endif()

if (WIN32)
  add_definitions("-DNOMINMAX")
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

target_include_directories(stompconn PUBLIC include/)

if (NOT STOMPCONN_LIBRARY_STATIC)
  if (STOMPCONN_WITH_STATIC_LIBEVENT)
    add_subdirectory(libevent)
  endif()
endif()

target_link_libraries(stompconn PRIVATE stomptalk event_core)
