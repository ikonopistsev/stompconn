TEMPLATE = lib
TARGET = stompconn

CONFIG -= qt
CONFIG += object_parallel_to_source c++17 console warn_on static
CONFIG -= app_bundle

SOURCES += \
    src/frame.cpp \
    src/handler.cpp \
    src/stomplay.cpp \
    src/connection.cpp \

HEADERS += \
    include/stompconn/handler.hpp \
    include/stompconn/packet.hpp \
    include/stompconn/stomplay.hpp \
    include/stompconn/frame.hpp \
    include/stompconn/connection.hpp \
    include/stompconn/header_store.hpp \

INCLUDEPATH += include \
    ..

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

QMAKE_CXXFLAGS_RELEASE += "-O3 -march=native"
QMAKE_LFLAGS_RELEASE += "-O3 -march=native"

#QMAKE_CXXFLAGS_RELEASE += "-O3 -march=native -fprofile-generate"
#QMAKE_LFLAGS_RELEASE += "-O3 -march=native -fprofile-generate"

#QMAKE_CXXFLAGS_RELEASE += "-O3 -march=native -fprofile-use"
#QMAKE_LFLAGS_RELEASE += "-O3 -march=native -fprofile-use"

