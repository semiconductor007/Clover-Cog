#include "gameclock.h"

#include <QtGlobal>
#include <limits>

GameClock::GameClock(int millisecondsPerGameMinute)
    : m_millisecondsPerGameMinute(qMax(1, millisecondsPerGameMinute)) {}

void GameClock::start() {
    m_partialMinuteMs = 0;
    m_timer.start();
}

void GameClock::pause() {
    if (!m_timer.isValid()) return;
    const qint64 ms = m_partialMinuteMs + m_timer.elapsed();
    m_gameMinuteOffset += ms / m_millisecondsPerGameMinute;
    m_partialMinuteMs = int(ms % m_millisecondsPerGameMinute);
    m_timer.invalidate();
}

void GameClock::resume() {
    if (!m_timer.isValid()) m_timer.start();
}

void GameClock::setElapsedGameMinutes(qint64 gameMinutes) {
    m_gameMinuteOffset = qMax<qint64>(0, gameMinutes);
    start();
}

qint64 GameClock::rawElapsedGameMinutes() const {
    if (!m_timer.isValid()) {
        return 0;
    }
    return (m_partialMinuteMs + m_timer.elapsed()) / m_millisecondsPerGameMinute;
}

qint64 GameClock::elapsedGameMinutes() const {
    return m_gameMinuteOffset + rawElapsedGameMinutes();
}

std::optional<qint64> GameClock::sleepTargetMinute(qint64 minutes, int wakeHour,
                                                   int sleepStartHour) {
    if (minutes < 0 || wakeHour < 1 || wakeHour > 23 ||
        sleepStartHour <= wakeHour || sleepStartHour > 23) return std::nullopt;
    const int minuteOfDay = int(minutes % 1440);
    const int wakeMinute = wakeHour * 60;
    if (minuteOfDay >= wakeMinute && minuteOfDay < sleepStartHour * 60) return std::nullopt;
    const int delta = minuteOfDay < wakeMinute ? wakeMinute - minuteOfDay
                                               : 1440 - minuteOfDay + wakeMinute;
    if (minutes > std::numeric_limits<qint64>::max() - delta) return std::nullopt;
    const qint64 target = minutes + delta;
    if (target / 1440 > std::numeric_limits<int>::max()) return std::nullopt;
    return target;
}

bool GameClock::canSleep(int sleepStartHour, int wakeHour) const {
    return sleepTargetMinute(elapsedGameMinutes(), wakeHour, sleepStartHour).has_value();
}

bool GameClock::sleepToNextMorning(int wakeHour, int sleepStartHour) {
    const qint64 current = elapsedGameMinutes();
    const auto target = sleepTargetMinute(current, wakeHour, sleepStartHour);
    if (!target) {
        return false;
    }
    setElapsedGameMinutes(*target);
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
