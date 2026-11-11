include(../CHECK.pro)

QT += testlib
CONFIG += testcase console
CONFIG -= app_bundle
TARGET = gameplay_panel_tests
VPATH += $$PWD/..
INCLUDEPATH += $$PWD/..
SOURCES -= main.cpp
SOURCES += gameplay_panel_tests.cpp
