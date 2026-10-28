#ifndef GAMECLOCK_H
#define GAMECLOCK_H

#include <QElapsedTimer>

class GameClock {
public:
    explicit GameClock(int millisecondsPerGameMinute = 1000);

    void start();
    void setElapsedGameMinutes(qint64 gameMinutes);
    bool canSleep(int sleepStartHour = 20) const;
    bool sleepToNextMorning(int wakeHour = 6, int sleepStartHour = 20);
    qint64 elapsedGameMinutes() const;
    int minute() const;
    int hour() const;
    int day() const;

    static int minuteFromElapsed(qint64 elapsedMilliseconds,
                                 int millisecondsPerGameMinute = 1000);
    static int hourFromElapsed(qint64 elapsedMilliseconds,
                               int millisecondsPerGameMinute = 1000);
    static int dayFromElapsed(qint64 elapsedMilliseconds,
                              int millisecondsPerGameMinute = 1000);

private:
    qint64 rawElapsedGameMinutes() const;
    QElapsedTimer m_timer;
    int m_millisecondsPerGameMinute;
    qint64 m_gameMinuteOffset = 0;
};

#endif // GAMECLOCK_H
