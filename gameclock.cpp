#include "gameclock.h"

#include <QtGlobal>

GameClock::GameClock(int millisecondsPerGameMinute)
    : m_millisecondsPerGameMinute(qMax(1, millisecondsPerGameMinute)) {}

void GameClock::start() {
    m_timer.start();
}

void GameClock::setElapsedGameMinutes(qint64 gameMinutes) {
    m_gameMinuteOffset = qMax<qint64>(0, gameMinutes);
    m_timer.restart();
}

qint64 GameClock::rawElapsedGameMinutes() const {
    if (!m_timer.isValid()) {
        return 0;
    }
    return m_timer.elapsed() / m_millisecondsPerGameMinute;
}

qint64 GameClock::elapsedGameMinutes() const {
    return m_gameMinuteOffset + rawElapsedGameMinutes();
}

namespace {

qint64 minutesUntilMorning(qint64 gameMinutes, int wakeHour, int sleepStartHour) {
    constexpr int dayLength = 24 * 60;
    const int minuteOfDay = static_cast<int>(gameMinutes % dayLength);
    const int wakeMinute = qBound(0, wakeHour, 23) * 60;
    const int sleepMinute = qBound(0, sleepStartHour, 23) * 60;
    if (minuteOfDay < wakeMinute) {
        return wakeMinute - minuteOfDay;
    }
    if (minuteOfDay >= sleepMinute) {
        return dayLength - minuteOfDay + wakeMinute;
    }
    return 0;
}

} // namespace

bool GameClock::canSleep(int sleepStartHour, int wakeHour) const {
    return minutesUntilMorning(elapsedGameMinutes(), wakeHour, sleepStartHour) > 0;
}

bool GameClock::sleepToNextMorning(int wakeHour, int sleepStartHour) {
    const qint64 current = elapsedGameMinutes();
    const qint64 duration = minutesUntilMorning(current, wakeHour, sleepStartHour);
    if (duration == 0) {
        return false;
    }
    setElapsedGameMinutes(current + duration);
    return true;
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
