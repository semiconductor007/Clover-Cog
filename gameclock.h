#ifndef GAMECLOCK_H
#define GAMECLOCK_H

#include <QElapsedTimer>
#include <optional>

class GameClock {
public:
    explicit GameClock(int millisecondsPerGameMinute = 1000);

    void start();
    void pause();
    void resume();
    bool isPaused() const { return m_paused; }
    void setElapsedGameMinutes(qint64 gameMinutes);
    // Any time may sleep to the next occurrence of the wake hour (strictly later).
    // Returns false without changing the clock for invalid input or overflow.
    bool sleepToNextMorning(int wakeHour = 6);
    static std::optional<qint64> sleepTargetMinute(qint64 currentMinutes,
                                                  int wakeHour = 6);
    // First 02:00 deadline crossed, or 'from' when resuming overdue before 06:00.
    static std::optional<qint64> passOutDeadline(qint64 from, qint64 to);
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
    bool m_paused = false;
};

#endif // GAMECLOCK_H
