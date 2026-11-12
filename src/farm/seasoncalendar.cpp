#include "seasoncalendar.h"

SeasonDate SeasonCalendar::dateForDay(qint64 zeroBasedGameDay) {
    const qint64 day = qMax<qint64>(0, zeroBasedGameDay);
    const qint64 seasonNumber = day / DaysPerSeason;
    return {seasonNumber / SeasonsPerYear + 1,
            static_cast<Season>(seasonNumber % SeasonsPerYear),
            static_cast<int>(day % DaysPerSeason) + 1};
}

SeasonDate SeasonCalendar::dateForMinutes(qint64 elapsedGameMinutes) {
    return dateForDay(qMax<qint64>(0, elapsedGameMinutes) / (24 * 60));
}

QString SeasonCalendar::seasonId(Season season) {
    switch (season) {
    case Season::Spring: return QStringLiteral("spring");
    case Season::Summer: return QStringLiteral("summer");
    case Season::Autumn: return QStringLiteral("autumn");
    case Season::Winter: return QStringLiteral("winter");
    }
    return {};
}

QString SeasonCalendar::seasonName(Season season) {
    switch (season) {
    case Season::Spring: return QStringLiteral("春季");
    case Season::Summer: return QStringLiteral("夏季");
    case Season::Autumn: return QStringLiteral("秋季");
    case Season::Winter: return QStringLiteral("冬季");
    }
    return {};
}
