#ifndef DAYNIGHTENVIRONMENT_H
#define DAYNIGHTENVIRONMENT_H

#include <QColor>
#include <QtGlobal>
#include "gamecalendar.h"

// Presentation only: no clock, calendar, inventory or farm state is owned here.
class DayNightEnvironment {
public:
    // Prototype season profiles share the same calendar as HUD and farm dates.
    static QColor tintAt(qint64 gameMinutes, bool outdoors = true, Season season = Season::Spring);
};

#endif
