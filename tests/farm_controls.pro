QT += widgets testlib
CONFIG += c++17 testcase console resources_big
TEMPLATE = app
TARGET = farm_controls_tests

INCLUDEPATH += ..
RESOURCES += ../character.qrc
SOURCES += farm_controls_tests.cpp \
    ../collisiondetector.cpp ../cropdefinition.cpp ../economyservice.cpp \
    ../inventory.cpp ../itembar.cpp ../mapdata.cpp ../maprenderer.cpp \
    ../mapwidget.cpp ../playercontroller.cpp ../rpg.cpp ../seasoncalendar.cpp ../worldstate.cpp
HEADERS += ../itembar.h ../mapwidget.h ../playercontroller.h ../rpg.h
