QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 resources_big

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    collisiondetector.cpp \
    gameclock.cpp \
    itembar.cpp \
    mapdata.cpp \
    mainwindow.cpp \
    maprenderer.cpp \
    mapwidget.cpp \
    playercontroller.cpp \
    rpg.cpp \
    worldstate.cpp

HEADERS += \
    collisiondetector.h \
    gameclock.h \
    gameconstants.h \
    itembar.h \
    mapdata.h \
    mainwindow.h \
    maprenderer.h \
    mapwidget.h \
    playercontroller.h \
    rpg.h \
    tooltype.h \
    worldstate.h

FORMS +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    character.qrc
