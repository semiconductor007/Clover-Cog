TEMPLATE = app
include(src/game.pri)
SOURCES += $$PWD/src/app/main.cpp

# Default install rules supplied by Qt Creator.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
