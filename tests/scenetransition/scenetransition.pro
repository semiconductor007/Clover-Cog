QT += widgets testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = scenetransition_tests

INCLUDEPATH += $$PWD/../../src
HEADERS += $$PWD/../../src/ui/scenetransition.h
SOURCES += $$PWD/../../src/ui/scenetransition.cpp scenetransition_tests.cpp
