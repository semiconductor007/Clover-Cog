#include "gameclock.h"

#include <QtGlobal>
#include <limits>

GameClock::GameClock(int millisecondsPerGameMinute)
    : m_millisecondsPerGameMinute(qMax(1, millisecondsPerGameMinute)) {}

void GameClock::start() {
    m_paused = false;
    m_timer.start();
}

void GameClock::pause() {
    if (m_paused) return;
    m_gameMinuteOffset = elapsedGameMinutes();
    m_paused = true;
    m_timer.invalidate();
}

void GameClock::resume() {
    if (!m_paused) return;
    m_paused = false;
    m_timer.start();
}

void GameClock::setElapsedGameMinutes(qint64 gameMinutes) {
    m_gameMinuteOffset = qMax<qint64>(0, gameMinutes);
    if (!m_paused) m_timer.restart();
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

std::optional<qint64> GameClock::sleepTargetMinute(qint64 currentMinutes,
                                                  int wakeHour) {
    // Project rule: civil days end at midnight, unlike Stardew's 26:00 day.
    constexpr int minutesPerDay = 24 * 60;
    if (currentMinutes < 0 || wakeHour < 1 || wakeHour > 23) {
        return std::nullopt;
    }
    const int minuteOfDay = static_cast<int>(currentMinutes % minutesPerDay);
    const int wakeMinute = wakeHour * 60;
    qint64 delta = 0;
    if (minuteOfDay < wakeMinute) {
        delta = wakeMinute - minuteOfDay;
    } else {
        delta = minutesPerDay - minuteOfDay + wakeMinute;
    }
    if (currentMinutes > std::numeric_limits<qint64>::max() - delta) {
        return std::nullopt;
    }
    return currentMinutes + delta;
}

bool GameClock::sleepToNextMorning(int wakeHour) {
    const auto target = sleepTargetMinute(elapsedGameMinutes(), wakeHour);
    if (!target) return false;
    setElapsedGameMinutes(*target);
    return true;
}

std::optional<qint64> GameClock::passOutDeadline(qint64 from, qint64 to) {
    if (from < 0 || to < from) return std::nullopt;
    const int minute = static_cast<int>(from % 1440);
    if (minute >= 120 && minute < 360) return from;
    const int delta = minute < 120 ? 120 - minute : 1440 - minute + 120;
    if (to - from < delta) return std::nullopt;
    return from + delta; // <= to: no overflow even near qint64 max.
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
