QT += core gui widgets multimedia testlib
CONFIG += c++17 testcase console resources_big
TEMPLATE = app
TARGET = sleep_ui_tests
DEFINES += CLOVER_COG_TESTING
INCLUDEPATH += ..
SOURCES += sleep_ui_tests.cpp \
    ../sleepsoundcue.cpp \
    ../seasonvisuals.cpp \
    ../gamecalendar.cpp ../calendarpresentation.cpp \
    ../daynightenvironment.cpp ../daynightoverlay.cpp \
    ../mainwindow.cpp ../gameclock.cpp ../worldstate.cpp ../mapdata.cpp \
    ../mapwidget.cpp ../maprenderer.cpp ../rpg.cpp ../playercontroller.cpp \
    ../collisiondetector.cpp ../inventory.cpp ../inventorypanel.cpp \
    ../economyservice.cpp ../cropdefinition.cpp ../savemanager.cpp \
    ../itembar.cpp ../seedshopdialog.cpp ../sleepoverlay.cpp
HEADERS += ../mainwindow.h ../mapwidget.h ../rpg.h ../playercontroller.h \
    ../sleepsoundcue.h \
    ../seasonvisuals.h \
    ../gamecalendar.h ../calendarpresentation.h \
    ../daynightenvironment.h ../daynightoverlay.h \
    ../itembar.h ../seedshopdialog.h ../inventorypanel.h ../sleepoverlay.h
RESOURCES += ../character.qrc
