#include "gamecalendar.h"

std::optional<CalendarDate> GameCalendar::forDay(qint64 absoluteDay, CalendarRules rules) {
    if (absoluteDay < 0 || !rules.isValid()) return std::nullopt;
    const qint64 totalSeasons = absoluteDay / rules.daysPerSeason;
    return CalendarDate{totalSeasons / 4 + 1,
                        static_cast<Season>(totalSeasons % 4),
                        static_cast<int>(absoluteDay % rules.daysPerSeason) + 1};
}

QString GameCalendar::seasonKey(Season season) {
    switch (season) {
    case Season::Spring: return QStringLiteral("spring");
    case Season::Summer: return QStringLiteral("summer");
    case Season::Autumn: return QStringLiteral("autumn");
    case Season::Winter: return QStringLiteral("winter");
    }
    return QStringLiteral("unknown");
}
