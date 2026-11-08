QT += core gui testlib
CONFIG += c++17 testcase console
TEMPLATE = app
TARGET = core_tests

INCLUDEPATH += ..

SOURCES += \
    core_tests.cpp \
    ../collisiondetector.cpp \
    ../cropdefinition.cpp \
    ../economyservice.cpp \
    ../gameclock.cpp \
    ../inventory.cpp \
    ../mapdata.cpp \
    ../savemanager.cpp \
    ../worldstate.cpp

HEADERS += \
    ../collisiondetector.h \
    ../cropdefinition.h \
    ../economyservice.h \
    ../gameclock.h \
    ../gameconstants.h \
    ../inventory.h \
    ../mapdata.h \
    ../savemanager.h \
    ../worldstate.h

# B: season conversion is shared by crop planting rules and time consumers.
SOURCES += ../seasoncalendar.cpp
HEADERS += ../seasoncalendar.h
