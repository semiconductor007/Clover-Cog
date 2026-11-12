#ifndef SLEEPSOUNDCUE_H
#define SLEEPSOUNDCUE_H

#include <QSoundEffect>

// One presentation cue per successful sleep attempt; never settles gameplay.
class SleepSoundCue : public QObject {
    Q_OBJECT
public:
    explicit SleepSoundCue(QObject *parent = nullptr);
    void beginAttempt();
    void complete(bool settled, bool saved);
    void finishAttempt();
    void setVolume(float volume);
    void setMuted(bool muted);
    void setSource(const QUrl &source);
    QSoundEffect::Status status() const { return m_effect.status(); }
signals:
    void successAccepted(); // Policy event, not evidence of audible output.
    void playbackRequested();
private:
    void tryPlay();
    QSoundEffect m_effect;
    bool m_accepted = false;
    bool m_pending = false;
    bool m_active = false;
};

#endif
