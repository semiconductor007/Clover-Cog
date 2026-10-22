#include "gameclock.h"

#include <QtGlobal>

GameClock::GameClock(int millisecondsPerGameMinute)
    : m_millisecondsPerGameMinute(qMax(1, millisecondsPerGameMinute)) {}

void GameClock::start() {
    m_timer.start();
}

qint64 GameClock::elapsedGameMinutes() const {
    if (!m_timer.isValid()) {
        return 0;
    }
    return m_timer.elapsed() / m_millisecondsPerGameMinute;
}

int GameClock::minute() const {
    return static_cast<int>(elapsedGameMinutes() % 60);
}

int GameClock::hour() const {
    return static_cast<int>((elapsedGameMinutes() / 60) % 24);
}

int GameClock::day() const {
    return static_cast<int>(elapsedGameMinutes() / (24 * 60));
}

int GameClock::minuteFromElapsed(qint64 elapsedMilliseconds,
                                 int millisecondsPerGameMinute) {
    const qint64 gameMinutes = elapsedMilliseconds / qMax(1, millisecondsPerGameMinute);
    return static_cast<int>(gameMinutes % 60);
}

int GameClock::hourFromElapsed(qint64 elapsedMilliseconds,
                               int millisecondsPerGameMinute) {
    const qint64 gameMinutes = elapsedMilliseconds / qMax(1, millisecondsPerGameMinute);
    return static_cast<int>((gameMinutes / 60) % 24);
}

int GameClock::dayFromElapsed(qint64 elapsedMilliseconds,
                              int millisecondsPerGameMinute) {
    const qint64 gameMinutes = elapsedMilliseconds / qMax(1, millisecondsPerGameMinute);
    return static_cast<int>(gameMinutes / (24 * 60));
}
