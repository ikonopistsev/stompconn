cmake_minimum_required(VERSION 3.10)

project(stompconn VERSION 1.6.7 LANGUAGES CXX)

option(STOMPCONN_DEBUG "show debug" OFF)
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
  src/stomplay/frame.cpp
  src/stomplay/subscription_handler.cpp
  src/stomplay/receipt_handler.cpp
  src/stomplay/client.cpp
  src/connection.cpp
  src/version.cpp
)

add_library(stompconn STATIC ${source})
target_include_directories(stompconn PUBLIC include/)

if (STOMPCONN_WITH_STATIC_LIBEVENT)
  add_subdirectory(libevent)
endif()

target_link_libraries(stompconn PRIVATE event_core stomptalk)

if (EVENT__HAVE_OPENSSL AND STOMPCONN_OPENSSL)
  target_link_libraries(stompconn PRIVATE event_openssl)
endif()
