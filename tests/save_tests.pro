QT += core gui testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = save_tests
INCLUDEPATH += $$PWD/../src

SOURCES += save_tests.cpp \
    $$PWD/../src/storage/savemanager.cpp \
    $$PWD/../src/economy/economyservice.cpp \
    $$PWD/../src/economy/inventory.cpp \
    $$PWD/../src/farm/worldstate.cpp \
    $$PWD/../src/farm/cropdefinition.cpp \
    $$PWD/../src/farm/seasoncalendar.cpp
