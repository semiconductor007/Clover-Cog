#ifndef GAMECLOCK_H
#define GAMECLOCK_H

#include "gameconstants.h"

#include <QElapsedTimer>
#include <optional>

class GameClock {
public:
    explicit GameClock(int millisecondsPerGameMinute = 1000);

    void start();
    void pause();
    void resume();
    bool isPaused() const { return !m_timer.isValid(); }
    static std::optional<qint64> sleepTargetMinute(qint64 minutes, int wakeHour = 6);
    static std::optional<qint64> passOutDeadline(qint64 from, qint64 to);
    void setElapsedGameMinutes(qint64 gameMinutes);
    bool canSleep(int sleepStartHour = GameConstants::SleepStartHour,
                  int wakeHour = GameConstants::WakeUpHour) const;
    bool sleepToNextMorning(int wakeHour = GameConstants::WakeUpHour,
                            int sleepStartHour = GameConstants::SleepStartHour);
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
    int m_partialMinuteMs = 0;
};

#endif // GAMECLOCK_H
