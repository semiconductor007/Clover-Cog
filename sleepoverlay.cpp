#include "sleepoverlay.h"

#include <QKeyEvent>
#include <QPainter>
#include <QPushButton>

SleepOverlay::SleepOverlay(QWidget *parent) : QWidget(parent) {
    m_calendarFont = CalendarPresentation::chooseFont();
    setGeometry(0, 0, 520, 320);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground);
    m_continue = new QPushButton(QStringLiteral("WAKE UP [ENTER]"), this);
    m_continue->setGeometry(157, 270, 206, 28);
    m_retry = new QPushButton(QStringLiteral("RETRY SAVE"), this);
    m_retry->setGeometry(193, 237, 134, 24);
    for (auto *button : {m_continue, m_retry}) {
        button->setStyleSheet(QStringLiteral(
            "QPushButton {color:#4c301e;background:#e8c98f;border:2px solid #8f6038;padding:2px;}"
            "QPushButton:focus {border:2px solid #fff1ba;}"));
        button->hide();
        button->setAutoRepeat(false);
        button->installEventFilter(this);
    }
    connect(m_continue, &QPushButton::clicked, this, &SleepOverlay::wakeUp);
    connect(m_retry, &QPushButton::clicked, this, [this]() {
        if (m_phase == Phase::Confirming) emit cancelled();
        else emit retrySaveRequested();
    });
    connect(&m_timer, &QTimer::timeout, this, &SleepOverlay::tick);
    hide();
}

void SleepOverlay::begin(bool passedOut) {
    if (m_phase != Phase::Hidden && m_phase != Phase::Confirming) return;
    m_continue->setGeometry(157, 270, 206, 28);
    m_retry->setGeometry(193, 237, 134, 24);
    m_phase = Phase::FadingOut;
    m_passedOut = passedOut;
    m_opacity = 0;
    m_continue->hide();
    m_retry->hide();
    m_retry->setText(QStringLiteral("RETRY SAVE"));
    m_elapsed.start();
    show();
    raise();
    setFocus();
    m_timer.start(16);
}

void SleepOverlay::showConfirmation(const QString &targetDate) {
    if (m_phase != Phase::Hidden) return;
    m_phase = Phase::Confirming;
    m_confirmationTarget = targetDate;
    m_continue->setGeometry(157, 182, 206, 28);
    m_retry->setGeometry(193, 220, 134, 24);
    m_continue->setText(QStringLiteral("SLEEP [ENTER]"));
    m_retry->setText(QStringLiteral("CANCEL [ESC]"));
    m_continue->show();
    m_retry->show();
    show();
    raise();
    setFocus();
}

void SleepOverlay::dismissConfirmation() {
    if (m_phase != Phase::Confirming) return;
    m_phase = Phase::Hidden;
    m_continue->hide();
    m_retry->hide();
    hide();
}

void SleepOverlay::showMorning(int oldDay, int newDay,
                               const DayAdvanceResult &result, bool saved, CalendarRules rules) {
    m_oldDay = oldDay;
    m_newDay = newDay;
    m_result = result;
    const auto oldDate = GameCalendar::forDay(oldDay, rules);
    const auto newDate = GameCalendar::forDay(newDay, rules);
    if (oldDate && newDate) {
        m_morningDateText = CalendarPresentation::dateText(*newDate, m_calendarFont.chinese)
            + QStringLiteral("  06:00");
        if (oldDay == newDay)
            m_dateTransitionText = m_calendarFont.chinese ? QStringLiteral("同日休息至早晨")
                                                        : QStringLiteral("RESTED UNTIL MORNING - SAME DAY");
        else if (oldDate->season != newDate->season || oldDate->year != newDate->year)
            m_dateTransitionText = m_calendarFont.chinese
                ? QStringLiteral("进入%1：%2").arg(CalendarPresentation::seasonName(newDate->season, true),
                    CalendarPresentation::dateText(*oldDate, true))
                : QStringLiteral("NEW SEASON: %1 > %2").arg(CalendarPresentation::dateText(*oldDate, false),
                    CalendarPresentation::seasonName(newDate->season, false));
        else
            m_dateTransitionText = CalendarPresentation::dateText(*oldDate, m_calendarFont.chinese)
                + QStringLiteral(" > ") + CalendarPresentation::dateText(*newDate, m_calendarFont.chinese);
    }
    m_phase = Phase::Morning;
    m_timer.stop();
    setSaveResult(saved);
    m_continue->show();
    m_continue->setFocus();
    update();
}

void SleepOverlay::setSaveResult(bool saved) {
    m_saved = saved;
    m_retry->setVisible(!saved && m_phase == Phase::Morning);
    m_continue->setText(saved ? QStringLiteral("WAKE UP [ENTER]")
                             : QStringLiteral("CONTINUE UNSAVED"));
    update();
}

void SleepOverlay::showFailure() {
    m_phase = Phase::Failed;
    m_timer.stop();
    m_opacity = 1;
    m_retry->setText(QStringLiteral("RETRY DAY"));
    m_retry->show();
    m_continue->setText(QStringLiteral("RETURN TO GAME"));
    m_continue->setVisible(!m_passedOut);
    if (m_passedOut) m_retry->setFocus();
    else m_continue->setFocus();
    update();
}

void SleepOverlay::wakeUp() {
    if (m_phase == Phase::Confirming) { emit confirmed(); return; }
    if (m_phase != Phase::Morning && m_phase != Phase::Failed) return;
    if (m_phase == Phase::Failed && m_passedOut) return;
    m_continue->hide();
    m_retry->hide();
    m_phase = Phase::FadingIn;
    m_elapsed.restart();
    setFocus();
    m_timer.start(16);
}

void SleepOverlay::tick() {
    const qreal progress = qMin<qreal>(1.0, m_elapsed.elapsed() / 650.0);
    if (m_phase == Phase::FadingOut) {
        m_opacity = progress;
        if (progress >= 1.0) {
            m_phase = Phase::Settling;
            m_timer.stop();
            update();
            emit settlementRequested();
        }
    } else if (m_phase == Phase::FadingIn) {
        m_opacity = 1.0 - progress;
        if (progress >= 1.0) {
            m_phase = Phase::Hidden;
            m_timer.stop();
            hide();
            emit finished();
        }
    }
    update();
}

void SleepOverlay::keyPressEvent(QKeyEvent *event) {
    if (m_phase == Phase::Confirming && event->key() == Qt::Key_Escape) {
        if (!event->isAutoRepeat()) emit cancelled();
        event->accept();
        return;
    }
    if (!event->isAutoRepeat() &&
        (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
         event->key() == Qt::Key_Space)) {
        wakeUp();
    }
    event->accept();
}

bool SleepOverlay::eventFilter(QObject *watched, QEvent *event) {
    if ((watched == m_continue || watched == m_retry) &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->isAutoRepeat()) return true;
        if (event->type() == QEvent::KeyPress &&
            (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)) {
            auto *button = static_cast<QPushButton *>(watched);
            if (!button->isHidden() && button->isEnabled()) button->click();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SleepOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    if (m_phase == Phase::Confirming) {
        p.fillRect(rect(), QColor(5, 9, 24, 180));
        p.fillRect(QRect(64, 64, 392, 194), QColor("#8f6038"));
        p.fillRect(QRect(68, 68, 384, 186), QColor("#e8c98f"));
        p.setPen(QColor("#4c301e"));
        p.drawText(QRect(80, 85, 360, 25), Qt::AlignCenter, QStringLiteral("REST UNTIL MORNING?"));
        p.drawText(QRect(80, 120, 360, 25), Qt::AlignCenter, m_confirmationTarget);
        p.drawText(QRect(80, 151, 360, 20), Qt::AlignCenter, QStringLiteral("TIME IS PAUSED WHILE CHOOSING"));
        return;
    }
    if (m_phase == Phase::Failed) {
        p.fillRect(rect(), QColor("#101b37"));
        p.setPen(QColor("#ffbc9b"));
        p.drawText(QRect(45, 70, 430, 35), Qt::AlignCenter, QStringLiteral("DAY SETTLEMENT FAILED"));
        p.setPen(Qt::white);
        const QStringList explanation{QStringLiteral("Morning was not completed."),
            QStringLiteral("Sleep time was restored."),
            QStringLiteral("The previous save has been preserved.")};
        for (int i = 0; i < explanation.size(); ++i)
            p.drawText(QRect(45, 114 + 27 * i, 430, 24), Qt::AlignCenter, explanation.at(i));
        return;
    }
    if (m_phase != Phase::Morning) {
        p.fillRect(rect(), QColor(5, 9, 24, qRound(255 * m_opacity)));
        if (m_passedOut) {
            p.setPen(QColor(255, 240, 186, qRound(255 * m_opacity)));
            p.drawText(QRect(70, 130, 380, 40), Qt::AlignCenter, QStringLiteral("YOU PASSED OUT"));
        }
        return;
    }
    p.fillRect(rect(), QColor("#101b37"));
    for (int i = 0; i < 34; ++i) {
        p.fillRect((i * 83 + 17) % 510, (i * 47 + 9) % 310,
                   i % 3 == 0 ? 2 : 1, 2, QColor("#b6c6dc"));
    }
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#fff0ba"));
    p.drawEllipse(QRect(420, 23, 28, 28));
    p.setBrush(QColor("#101b37"));
    p.drawEllipse(QRect(430, 18, 26, 26));
    p.setPen(QColor("#fff0ba"));
    p.drawText(QRect(70, 18, 380, 23), Qt::AlignCenter,
               m_passedOut ? QStringLiteral("PASSED OUT - WOKE AT BED") : QStringLiteral("A NEW MORNING"));
    p.fillRect(QRect(64, 49, 392, 161), QColor("#8f6038"));
    p.fillRect(QRect(68, 53, 384, 153), QColor("#e8c98f"));
    p.setPen(QColor("#4c301e"));
    p.save();
    p.setFont(m_calendarFont.font);
    p.drawText(QRect(80, 62, 360, 23), Qt::AlignCenter,
               m_morningDateText);
    p.drawText(QRect(80, 89, 360, 18), Qt::AlignCenter,
               m_dateTransitionText);
    p.restore();
    const QStringList rows{
        QStringLiteral("CROPS GREW: %1").arg(m_result.cropsAdvanced),
        QStringLiteral("NO WATER / PAUSED: %1").arg(m_result.cropsStalled),
        QStringLiteral("EMPTY SOIL RECOVERED: %1").arg(m_result.soilRecovered)};
    for (int i = 0; i < rows.size(); ++i)
        p.drawText(QRect(94, 117 + 25 * i, 335, 21), Qt::AlignVCenter, rows.at(i));
    p.setPen(m_saved ? QColor("#b7e4a5") : QColor("#ffbc9b"));
    p.drawText(QRect(60, 211, 400, 23), Qt::AlignCenter,
               m_saved ? QStringLiteral("PROGRESS SAVED") : QStringLiteral("SAVE FAILED - PROGRESS NOT SAVED"));
}
