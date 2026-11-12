QT += core gui testlib
CONFIG += console c++17 testcase resources_big
CONFIG -= app_bundle
TEMPLATE = app
TARGET = seasonappearance_tests

INCLUDEPATH += $$PWD/../../src
SOURCES += seasonappearance_tests.cpp \
    $$PWD/../../src/map/seasonappearance.cpp \
    $$PWD/../../src/map/mapdata.cpp \
    $$PWD/../../src/farm/worldstate.cpp \
    $$PWD/../../src/farm/cropdefinition.cpp \
    $$PWD/../../src/farm/seasoncalendar.cpp
RESOURCES += ../../character.qrc
