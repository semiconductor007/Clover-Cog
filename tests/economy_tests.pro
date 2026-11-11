QT += core gui testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = economy_tests
INCLUDEPATH += ..

SOURCES += economy_tests.cpp \
           ../cropdefinition.cpp \
           ../economyservice.cpp \
           ../inventory.cpp \
           ../seasoncalendar.cpp \
           ../worldstate.cpp
