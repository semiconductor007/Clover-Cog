QT += widgets testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = scenetransition_tests

INCLUDEPATH += ../..
HEADERS += ../../scenetransition.h
SOURCES += ../../scenetransition.cpp scenetransition_tests.cpp
