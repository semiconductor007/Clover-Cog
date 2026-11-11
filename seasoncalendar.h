#ifndef SEASONCALENDAR_H
#define SEASONCALENDAR_H

#include <QString>
#include <QtGlobal>

enum class Season { Spring = 0, Summer = 1, Autumn = 2, Winter = 3 };

struct SeasonDate {
    qint64 year = 1;
    Season season = Season::Spring;
    int dayOfSeason = 1;
};

// Pure date conversion. GameClock remains the only owner of elapsed time.
class SeasonCalendar {
public:
    static constexpr int DaysPerSeason = 7;
    static constexpr int SeasonsPerYear = 4;
    static constexpr quint8 AllSeasons = 0x0f;

    static SeasonDate dateForDay(qint64 zeroBasedGameDay);
    static SeasonDate dateForMinutes(qint64 elapsedGameMinutes);
    static QString seasonId(Season season);
    static QString seasonName(Season season);
    static constexpr quint8 seasonBit(Season season) {
        const int index = static_cast<int>(season);
        return index >= 0 && index < SeasonsPerYear
                   ? static_cast<quint8>(1u << index) : 0;
    }
};

#endif // SEASONCALENDAR_H
