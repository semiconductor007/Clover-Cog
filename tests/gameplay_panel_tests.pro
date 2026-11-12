include(../src/game.pri)

QT += testlib
CONFIG += testcase console
CONFIG -= app_bundle
TARGET = gameplay_panel_tests
SOURCES += gameplay_panel_tests.cpp
