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

namespace {

qint64 minutesUntilMorning(qint64 gameMinutes, int wakeHour, int sleepStartHour) {
    Q_UNUSED(sleepStartHour); // Retained for source compatibility; daytime rest is now allowed.
    const auto target = GameClock::sleepTargetMinute(gameMinutes, wakeHour);
    return target ? *target - gameMinutes : 0;
}

} // namespace

std::optional<qint64> GameClock::sleepTargetMinute(qint64 minutes, int wakeHour) {
    if (minutes < 0 || wakeHour < 1 || wakeHour > 23) return std::nullopt;
    const int minute = int(minutes % 1440), wake = wakeHour * 60;
    const int delta = minute < wake ? wake - minute : 1440 - minute + wake;
    if (minutes > std::numeric_limits<qint64>::max() - delta) return std::nullopt;
    return minutes + delta;
}

std::optional<qint64> GameClock::passOutDeadline(qint64 from, qint64 to) {
    if (from < 0 || to < from) return std::nullopt;
    const int minute = int(from % 1440);
    if (minute >= 120 && minute < 360) return from;
    const int delta = minute < 120 ? 120 - minute : 1440 - minute + 120;
    return to - from >= delta ? std::optional<qint64>(from + delta) : std::nullopt;
}

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
