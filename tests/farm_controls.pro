QT += widgets testlib
CONFIG += c++17 testcase console resources_big
TEMPLATE = app
TARGET = farm_controls_tests

INCLUDEPATH += $$PWD/../src
RESOURCES += ../character.qrc
SOURCES += farm_controls_tests.cpp \
    $$PWD/../src/player/collisiondetector.cpp $$PWD/../src/farm/cropdefinition.cpp $$PWD/../src/economy/economyservice.cpp \
    $$PWD/../src/economy/inventory.cpp $$PWD/../src/ui/itembar.cpp $$PWD/../src/map/mapdata.cpp $$PWD/../src/map/maprenderer.cpp \
    $$PWD/../src/map/mapwidget.cpp $$PWD/../src/player/playercontroller.cpp $$PWD/../src/player/rpg.cpp $$PWD/../src/farm/seasoncalendar.cpp $$PWD/../src/farm/worldstate.cpp
HEADERS += $$PWD/../src/ui/itembar.h $$PWD/../src/map/mapwidget.h $$PWD/../src/player/playercontroller.h $$PWD/../src/player/rpg.h
