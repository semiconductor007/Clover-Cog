# Shared by the game and the tests that build its full window.
QT += core gui widgets multimedia
CONFIG += c++17 resources_big
INCLUDEPATH += $$PWD

SOURCES += $$PWD/app/mainwindow.cpp \
    $$PWD/core/gameclock.cpp \
    $$PWD/economy/economyservice.cpp \
    $$PWD/economy/inventory.cpp \
    $$PWD/farm/cropdefinition.cpp \
    $$PWD/farm/seasoncalendar.cpp \
    $$PWD/farm/worldstate.cpp \
    $$PWD/map/mapdata.cpp \
    $$PWD/map/maprenderer.cpp \
    $$PWD/map/mapwidget.cpp \
    $$PWD/map/seasonappearance.cpp \
    $$PWD/player/collisiondetector.cpp \
    $$PWD/player/playercontroller.cpp \
    $$PWD/player/rpg.cpp \
    $$PWD/storage/savemanager.cpp \
    $$PWD/ui/farmmenus.cpp \
    $$PWD/ui/inventorydialog.cpp \
    $$PWD/ui/itembar.cpp \
    $$PWD/ui/marketdialog.cpp \
    $$PWD/ui/scenetransition.cpp \
    $$PWD/ui/seedshopdialog.cpp \
    $$PWD/ui/storagedialog.cpp

HEADERS += $$PWD/app/mainwindow.h \
    $$PWD/core/gameclock.h \
    $$PWD/core/gameconstants.h \
    $$PWD/economy/economyservice.h \
    $$PWD/economy/inventory.h \
    $$PWD/farm/cropdefinition.h \
    $$PWD/farm/seasoncalendar.h \
    $$PWD/farm/worldstate.h \
    $$PWD/map/mapdata.h \
    $$PWD/map/maprenderer.h \
    $$PWD/map/mapwidget.h \
    $$PWD/map/seasonappearance.h \
    $$PWD/player/collisiondetector.h \
    $$PWD/player/playercontroller.h \
    $$PWD/player/rpg.h \
    $$PWD/player/tooltype.h \
    $$PWD/storage/savemanager.h \
    $$PWD/ui/farmicons.h \
    $$PWD/ui/farmmenus.h \
    $$PWD/ui/farmpanel.h \
    $$PWD/ui/inventorydialog.h \
    $$PWD/ui/itembar.h \
    $$PWD/ui/marketdialog.h \
    $$PWD/ui/scenetransition.h \
    $$PWD/ui/seedshopdialog.h \
    $$PWD/ui/storagedialog.h

RESOURCES += $$PWD/../character.qrc $$PWD/../farm-sounds.qrc
