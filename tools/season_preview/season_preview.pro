QT += core gui
CONFIG += console c++17 resources_big
CONFIG -= app_bundle
TEMPLATE = app
TARGET = season_preview

INCLUDEPATH += ../..
SOURCES += main.cpp \
    ../../mapdata.cpp \
    ../../maprenderer.cpp \
    ../../worldstate.cpp \
    ../../cropdefinition.cpp \
    ../../seasoncalendar.cpp
HEADERS += ../../mapdata.h ../../maprenderer.h ../../worldstate.h \
    ../../cropdefinition.h ../../seasoncalendar.h ../../gameconstants.h
RESOURCES += ../../character.qrc
