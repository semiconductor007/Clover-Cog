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
    const qint64 absoluteMinutes = m_gameMinuteOffset + rawElapsedGameMinutes();
    // A player may let the evening clock run out, but the calendar day does
    // not roll over until the player explicitly sleeps.
    const qint64 logicalDayEnd =
        (m_gameMinuteOffset / (24 * 60) + 1) * (24 * 60) - 1;
    return qMin(absoluteMinutes, logicalDayEnd);
}

bool GameClock::canSleep(int sleepStartHour) const {
    const int boundedSleepStart = qBound(0, sleepStartHour, 23);
    return hour() >= boundedSleepStart;
}

bool GameClock::sleepToNextMorning(int wakeHour, int sleepStartHour) {
    if (!canSleep(sleepStartHour)) {
        return false;
    }
    const int boundedWakeHour = qBound(0, wakeHour, 23);
    const qint64 current = elapsedGameMinutes();
    const qint64 nextMorning = (current / (24 * 60) + 1) * 24 * 60
                               + boundedWakeHour * 60;
    m_gameMinuteOffset = nextMorning;
    m_timer.restart();
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
