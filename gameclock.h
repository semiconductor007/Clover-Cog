#ifndef GAMECLOCK_H
#define GAMECLOCK_H

#include <QElapsedTimer>

class GameClock {
public:
    explicit GameClock(int millisecondsPerGameMinute = 1000);

    void start();
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
    QElapsedTimer m_timer;
    int m_millisecondsPerGameMinute;
};

#endif // GAMECLOCK_H
