QT += core gui
CONFIG += console c++17 resources_big
CONFIG -= app_bundle
TEMPLATE = app
TARGET = season_preview

INCLUDEPATH += $$PWD/../../src
SOURCES += main.cpp \
    $$PWD/../../src/map/mapdata.cpp \
    $$PWD/../../src/map/maprenderer.cpp \
    $$PWD/../../src/farm/worldstate.cpp \
    $$PWD/../../src/farm/cropdefinition.cpp \
    $$PWD/../../src/farm/seasoncalendar.cpp
HEADERS += $$PWD/../../src/map/mapdata.h $$PWD/../../src/map/maprenderer.h $$PWD/../../src/farm/worldstate.h \
    $$PWD/../../src/farm/cropdefinition.h $$PWD/../../src/farm/seasoncalendar.h $$PWD/../../src/core/gameconstants.h
RESOURCES += ../../character.qrc
