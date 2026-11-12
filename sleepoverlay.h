#ifndef SLEEPOVERLAY_H
#define SLEEPOVERLAY_H

#include "worldstate.h"
#include "calendarpresentation.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

class QPushButton;

// Presentation only. The opaque checkpoint asks MainWindow to settle/save once.
class SleepOverlay : public QWidget {
    Q_OBJECT
public:
    enum class Phase { Hidden, Confirming, FadingOut, Settling, Morning, Failed, FadingIn };
    explicit SleepOverlay(QWidget *parent = nullptr);
    void begin(bool passedOut = false);
    void showConfirmation(const QString &targetDate);
    void dismissConfirmation();
    void showMorning(int oldDay, int newDay, const DayAdvanceResult &result,
                     bool saved, CalendarRules rules = {});
    QString morningDateText() const { return m_morningDateText; }
    QString dateTransitionText() const { return m_dateTransitionText; }
    void setSaveResult(bool saved);
    void showFailure();
    Phase phase() const { return m_phase; }
signals:
    void settlementRequested();
    void finished();
    void retrySaveRequested();
    void confirmed();
    void cancelled();
protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void tick();
    void wakeUp();
    Phase m_phase = Phase::Hidden;
    QElapsedTimer m_elapsed;
    QTimer m_timer;
    qreal m_opacity = 0;
    int m_oldDay = 0;
    int m_newDay = 0;
    DayAdvanceResult m_result;
    bool m_saved = false;
    bool m_passedOut = false;
    CalendarFont m_calendarFont;
    QString m_morningDateText;
    QString m_dateTransitionText;
    QString m_confirmationTarget;
    QPushButton *m_continue;
    QPushButton *m_retry;
};
#endif
