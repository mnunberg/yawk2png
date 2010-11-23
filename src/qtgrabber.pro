# -------------------------------------------------
# Project created by QtCreator 2010-11-19T20:36:39
# -------------------------------------------------
QT += webkit \
    network
TARGET = qtgrabber
TEMPLATE = app
SOURCES += main.cpp \
    webkitrenderer.cpp \
    configurablepage.cpp \
    twutil.cpp \
    customnam.cpp
HEADERS += webkitrenderer.h \
    configurablepage.h \
    twutil.h \
    customnam.h
QMAKE_CXXFLAGS += -g \
    -D_GNU_SOURCE
