#include "sleepsoundcue.h"
#include <QMediaDevices>
#include <QAudioDevice>

SleepSoundCue::SleepSoundCue(QObject *parent) : QObject(parent) {
    m_effect.setLoopCount(1);
    m_effect.setVolume(0.35f);
    connect(&m_effect, &QSoundEffect::statusChanged, this, &SleepSoundCue::tryPlay);
    m_effect.setSource(QUrl(QStringLiteral("qrc:/sounds/wav/sleep_success.wav")));
}

void SleepSoundCue::beginAttempt() {
    m_effect.stop();
    m_active = true;
    m_accepted = false;
    m_pending = false;
}

void SleepSoundCue::complete(bool settled, bool saved) {
    if (!m_active || !settled || !saved || m_accepted) return;
    m_accepted = true;
    m_pending = !m_effect.isMuted() && m_effect.volume() > 0;
    emit successAccepted();
    tryPlay();
}

void SleepSoundCue::finishAttempt() {
    m_active = false;
    m_pending = false; // Never play a late-loaded cue after leaving the summary.
}

void SleepSoundCue::setVolume(float volume) {
    m_effect.setVolume(qBound(0.0f, volume, 1.0f));
    if (m_effect.volume() == 0) m_pending = false;
}

void SleepSoundCue::setMuted(bool muted) {
    m_effect.setMuted(muted);
    if (muted) m_pending = false;
}

void SleepSoundCue::setSource(const QUrl &source) {
    m_effect.setSource(source);
}

void SleepSoundCue::tryPlay() {
    if (!m_active || !m_pending || m_effect.status() != QSoundEffect::Ready ||
        m_effect.isMuted() || m_effect.volume() <= 0) return;
    m_pending = false;
    if (QMediaDevices::defaultAudioOutput().isNull()) return;
    emit playbackRequested();
    m_effect.play();
}
