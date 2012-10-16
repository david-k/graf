TEMPLATE = app
CONFIG += console
CONFIG -= qt

ROOT_DIR = ../..
LIGHT_DIR = $${ROOT_DIR}/../light
RED_DIR = $${ROOT_DIR}/../red
BINARY_BASE = $${ROOT_DIR}/bin
OBJECT_BASE = $${ROOT_DIR}/obj

INCLUDEPATH += $${ROOT_DIR}
INCLUDEPATH += $${LIGHT_DIR}
INCLUDEPATH += $${RED_DIR}

# Build targets
CONFIG(debug, debug|release) {
    # Debug
    DEFINES += _DEBUG

    # Output dirs
    win32:DESTDIR = $${BINARY_BASE}/windows/debug
    win32:OBJECTS_DIR = $${OBJECT_BASE}/windows/debug
    unix:DESTDIR = $${BINARY_BASE}/linux/debug
    unix:OBJECTS_DIR = $${OBJECT_BASE}/linux/debug

    # Libs
    unix:LIBS += $${LIGHT_DIR}/bin/linux/debug/liblight.a
} else {
    # Release
    # Output dirs
    win32:DESTDIR = $${BINARY_BASE}/windows/release
    win32:OBJECTS_DIR = $${OBJECT_BASE}/windows/release
    unix:DESTDIR = $${BINARY_BASE}/linux/release
    unix:OBJECTS_DIR = $${OBJECT_BASE}/linux/release

    # Libs
    unix:LIBS += $${LIGHT_DIR}/bin/linux/release/liblight.a
}

# Compiler options
QMAKE_CXXFLAGS_RELEASE += -std=c++0x -Wall #-S -masm=intel
QMAKE_CXXFLAGS_DEBUG += -std=c++0x -Wall

# Libs
unix:LIBS += -L/usr/lib
unix:LIBS += -lrt
unix:LIBS += -lX11
unix:LIBS += -lGL

SOURCES += \
    ../../test/main.cpp \
    ../../graf/internal/windows_opengl_context.cpp

HEADERS += \
    ../../graf/internal/linux_window.hpp \
    ../../graf/graf.hpp \
    ../../graf/opengl_context.hpp \
    ../../graf/internal/linux_opengl_context.hpp


