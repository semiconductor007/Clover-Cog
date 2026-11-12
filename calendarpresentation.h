#ifndef CALENDARPRESENTATION_H
#define CALENDARPRESENTATION_H

#include "gamecalendar.h"
#include <QFont>

struct CalendarFont {
    QFont font;
    bool chinese = false;
};

namespace CalendarPresentation {
CalendarFont chooseFont();
QString seasonName(Season season, bool chinese);
QString dateText(const CalendarDate &date, bool chinese, bool twoLines = false);
}

#endif
