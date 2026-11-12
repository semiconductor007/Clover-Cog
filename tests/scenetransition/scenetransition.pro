QT += widgets testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = scenetransition_tests

INCLUDEPATH += $$PWD/../../src
HEADERS += $$PWD/../../src/ui/scenetransition.h \
    $$PWD/../../src/farm/seasoncalendar.h
SOURCES += $$PWD/../../src/ui/scenetransition.cpp \
    $$PWD/../../src/farm/seasoncalendar.cpp \
    scenetransition_tests.cpp
