include(../../src/game.pri)

QT += testlib
CONFIG += testcase console
CONFIG -= app_bundle
TARGET = clock_ui_tests
SOURCES += clock_ui_tests.cpp
