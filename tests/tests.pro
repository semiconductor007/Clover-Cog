QT += core gui testlib
CONFIG += c++17 testcase console
TEMPLATE = app
TARGET = core_tests

INCLUDEPATH += $$PWD/../src

SOURCES += \
    core_tests.cpp \
    $$PWD/../src/player/collisiondetector.cpp \
    $$PWD/../src/farm/cropdefinition.cpp \
    $$PWD/../src/economy/economyservice.cpp \
    $$PWD/../src/core/gameclock.cpp \
    $$PWD/../src/economy/inventory.cpp \
    $$PWD/../src/map/mapdata.cpp \
    $$PWD/../src/storage/savemanager.cpp \
    $$PWD/../src/farm/worldstate.cpp

HEADERS += \
    $$PWD/../src/player/collisiondetector.h \
    $$PWD/../src/farm/cropdefinition.h \
    $$PWD/../src/economy/economyservice.h \
    $$PWD/../src/core/gameclock.h \
    $$PWD/../src/core/gameconstants.h \
    $$PWD/../src/economy/inventory.h \
    $$PWD/../src/map/mapdata.h \
    $$PWD/../src/storage/savemanager.h \
    $$PWD/../src/farm/worldstate.h

# B: season conversion is shared by crop planting rules and time consumers.
SOURCES += $$PWD/../src/farm/seasoncalendar.cpp
HEADERS += $$PWD/../src/farm/seasoncalendar.h
