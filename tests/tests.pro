QT += core gui testlib
CONFIG += c++17 testcase console
TEMPLATE = app
TARGET = core_tests

INCLUDEPATH += ..

SOURCES += \
    core_tests.cpp \
    ../collisiondetector.cpp \
    ../gameclock.cpp \
    ../mapdata.cpp \
    ../worldstate.cpp

HEADERS += \
    ../collisiondetector.h \
    ../gameclock.h \
    ../gameconstants.h \
    ../mapdata.h \
    ../worldstate.h
