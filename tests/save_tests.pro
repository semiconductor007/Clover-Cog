QT += core gui testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = save_tests
INCLUDEPATH += ..

SOURCES += save_tests.cpp \
    ../savemanager.cpp \
    ../economyservice.cpp \
    ../inventory.cpp \
    ../worldstate.cpp \
    ../cropdefinition.cpp \
    ../seasoncalendar.cpp
