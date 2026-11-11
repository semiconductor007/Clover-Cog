QT += core gui widgets testlib
CONFIG += c++17 testcase console resources_big
CONFIG -= app_bundle
TEMPLATE = app
TARGET = panel_tests
INCLUDEPATH += ..
SOURCES += panel_tests.cpp \
    ../cropdefinition.cpp ../seasoncalendar.cpp ../inventory.cpp ../economyservice.cpp \
    ../inventorydialog.cpp ../seedshopdialog.cpp ../marketdialog.cpp
HEADERS += ../farmpanel.h ../inventorydialog.h ../seedshopdialog.h ../marketdialog.h
RESOURCES += ../character.qrc
