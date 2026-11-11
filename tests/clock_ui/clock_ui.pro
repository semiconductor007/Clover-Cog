farmRoot = $$clean_path($$PWD/../..)
include($$farmRoot/CHECK.pro)

QT += testlib
CONFIG += testcase console
CONFIG -= app_bundle
TARGET = clock_ui_tests

INCLUDEPATH += $$farmRoot
VPATH += $$farmRoot
SOURCES -= main.cpp
SOURCES += clock_ui_tests.cpp
