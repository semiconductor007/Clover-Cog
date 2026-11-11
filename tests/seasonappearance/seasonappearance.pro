QT += core gui testlib
CONFIG += console c++17 testcase resources_big
CONFIG -= app_bundle
TEMPLATE = app
TARGET = seasonappearance_tests

INCLUDEPATH += ../..
SOURCES += seasonappearance_tests.cpp \
    ../../seasonappearance.cpp \
    ../../mapdata.cpp \
    ../../worldstate.cpp \
    ../../cropdefinition.cpp \
    ../../seasoncalendar.cpp
RESOURCES += ../../character.qrc
