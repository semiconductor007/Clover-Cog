QT += core gui testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = economy_tests
INCLUDEPATH += $$PWD/../src

SOURCES += economy_tests.cpp \
           $$PWD/../src/farm/cropdefinition.cpp \
           $$PWD/../src/economy/economyservice.cpp \
           $$PWD/../src/economy/inventory.cpp \
           $$PWD/../src/farm/seasoncalendar.cpp \
           $$PWD/../src/farm/worldstate.cpp
