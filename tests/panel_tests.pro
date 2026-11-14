QT += core gui widgets testlib
CONFIG += c++17 testcase console resources_big
CONFIG -= app_bundle
TEMPLATE = app
TARGET = panel_tests
INCLUDEPATH += $$PWD/../src
SOURCES += panel_tests.cpp \
    $$PWD/../src/farm/cropdefinition.cpp $$PWD/../src/farm/seasoncalendar.cpp $$PWD/../src/economy/inventory.cpp $$PWD/../src/economy/economyservice.cpp \
    $$PWD/../src/ui/inventorydialog.cpp $$PWD/../src/ui/seedshopdialog.cpp $$PWD/../src/ui/marketdialog.cpp $$PWD/../src/ui/storagedialog.cpp
HEADERS += $$PWD/../src/ui/farmpanel.h $$PWD/../src/ui/inventorydialog.h $$PWD/../src/ui/seedshopdialog.h $$PWD/../src/ui/marketdialog.h $$PWD/../src/ui/storagedialog.h
HEADERS += $$PWD/../src/ui/slotgrid.h
RESOURCES += ../character.qrc
