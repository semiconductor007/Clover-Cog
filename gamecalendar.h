#ifndef GAMECALENDAR_H
#define GAMECALENDAR_H

#include <QString>
#include <optional>

enum class Season { Spring, Summer, Autumn, Winter };

struct CalendarDate {
    qint64 year = 1;
    Season season = Season::Spring;
    int dayOfSeason = 1;
};

struct CalendarRules {
    // Trial value from the team plan, not final balance. Existing saves pin it.
    static constexpr int PrototypeDaysPerSeason = 7;
    static constexpr int LegacyDaysPerSeason = 7;
    int daysPerSeason = PrototypeDaysPerSeason;
    bool isValid() const { return daysPerSeason > 0 && daysPerSeason <= 366; }
};

// Pure shared calendar contract (B); no clock, farm or save state is owned here.
class GameCalendar {
public:
    static std::optional<CalendarDate> forDay(qint64 absoluteDay, CalendarRules rules = {});
    static QString seasonKey(Season season);
};

#endif
