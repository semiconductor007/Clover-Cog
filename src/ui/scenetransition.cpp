#include "scenetransition.h"
#include "farm/seasoncalendar.h"

#include <QEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPushButton>
#include <QVariantAnimation>
#include <array>

namespace {

constexpr int FadeToBlackMs = 350;
constexpr int HoldBlackMs = 100;
constexpr int RevealSceneMs = 450;

struct LightStop {
    int minute;
    QColor color;
};

// Preserve over half the terrain contrast even at the darkest point.
const QColor night(24, 36, 78, 124);
const QColor sunlight(250, 203, 150, 0);
struct LightSchedule {
    int dawnStart;
    int sunrise;
    int daylight;
    int duskStart;
};

LightSchedule scheduleForSeason(Season season) {
    // Trial timings: summer has the longest day. Keep winter's 06:00 wake-up
    // as readable as the original spring morning, with a shorter dawn.
    switch (season) {
    case Season::Summer: return {3 * 60, 5 * 60, 7 * 60, 18 * 60};
    case Season::Autumn: return {4 * 60 + 30, 6 * 60, 8 * 60, 16 * 60};
    case Season::Winter: return {5 * 60, 6 * 60, 8 * 60, 15 * 60};
    case Season::Spring: return {4 * 60, 6 * 60, 8 * 60, 16 * 60 + 30};
    }
    return {4 * 60, 6 * 60, 8 * 60, 16 * 60 + 30};
}

std::array<LightStop, 9> lightStopsForSeason(Season season) {
    const LightSchedule schedule = scheduleForSeason(season);
    // Retain the spring palette and the time between each evening colour.
    // Every season reaches the same night tint before midnight, including
    // the last day of a season/year, so the new date cannot flash brighter.
    return {{{0, night},
             {schedule.dawnStart, night},
             {schedule.sunrise, QColor(250, 203, 150, 48)},
             {schedule.daylight, sunlight},
             {schedule.duskStart, sunlight},
             {schedule.duskStart + 2 * 60, QColor(223, 139, 107, 58)},
             {schedule.duskStart + 3 * 60 + 30, QColor(79, 69, 112, 94)},
             {schedule.duskStart + 5 * 60 + 30, night},
             {24 * 60, night}}};
}

} // namespace

CanvasOverlay::CanvasOverlay(QWidget *canvas) : QWidget(canvas) {
    setFocusPolicy(Qt::NoFocus);
    setGeometry(canvas->rect());
    canvas->installEventFilter(this);
}

bool CanvasOverlay::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
    }
    return QWidget::eventFilter(watched, event);
}

DayNightOverlay::DayNightOverlay(QWidget *canvas) : CanvasOverlay(canvas) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

QColor DayNightOverlay::tintForMinutes(qint64 minutes) {
    const qint64 elapsedMinutes = qMax<qint64>(0, minutes);
    const auto lightStops = lightStopsForSeason(SeasonCalendar::dateForMinutes(elapsedMinutes).season);
    const int minuteOfDay = int(elapsedMinutes % (24 * 60));
    for (size_t i = 1; i < lightStops.size(); ++i) {
        const LightStop &from = lightStops[i - 1];
        const LightStop &to = lightStops[i];
        if (minuteOfDay <= to.minute) {
            const qreal progress = qreal(minuteOfDay - from.minute) / (to.minute - from.minute);
            const auto blend = [progress](int a, int b) { return qRound(a + (b - a) * progress); };
            return QColor(blend(from.color.red(), to.color.red()),
                          blend(from.color.green(), to.color.green()),
                          blend(from.color.blue(), to.color.blue()),
                          blend(from.color.alpha(), to.color.alpha()));
        }
    }
    return night;
}

void DayNightOverlay::setGameMinutes(qint64 minutes) {
    const QColor tint = tintForMinutes(minutes);
    if (tint != m_tint) {
        m_tint = tint;
        update();
    }
}

void DayNightOverlay::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), m_tint);
}

SceneTransition::SceneTransition(QWidget *canvas)
    : CanvasOverlay(canvas), m_animation(this) {
    hide();
    setAttribute(Qt::WA_NoMousePropagation);
    const auto addFade = [this](int from, int to, int duration) {
        auto *fade = new QVariantAnimation;
        fade->setStartValue(from);
        fade->setEndValue(to);
        fade->setDuration(duration);
        connect(fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            m_blackAlpha = value.toInt();
            update();
        });
        m_animation.addAnimation(fade);
        return fade;
    };

    auto *fadeToBlack = addFade(0, 255, FadeToBlackMs);
    m_animation.addPause(HoldBlackMs);
    addFade(255, 0, RevealSceneMs);
    connect(fadeToBlack, &QVariantAnimation::finished, this, [this]() {
        m_outgoingFrame = QPixmap();
        emit blackReached();
    });
    connect(&m_animation, &QSequentialAnimationGroup::finished, this, [this]() {
        hide();
        emit finished();
    });
}

bool SceneTransition::isActive() const {
    return m_animation.state() != QAbstractAnimation::Stopped;
}

bool SceneTransition::start() {
    if (isActive()) {
        return false;
    }
    m_blackAlpha = 0;
    m_outgoingFrame = parentWidget()->grab();
    raise();
    show();
    m_animation.start();
    return true;
}

void SceneTransition::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    if (!m_outgoingFrame.isNull()) {
        painter.drawPixmap(rect(), m_outgoingFrame);
    }
    painter.fillRect(rect(), QColor(0, 0, 0, m_blackAlpha));
}

namespace {
QRectF sleepPageRect(const QSize &viewport) {
    const qreal scale = qMin(viewport.width() / 520.0, viewport.height() / 320.0);
    const QSizeF size(520 * scale, 320 * scale);
    return QRectF(QPointF((viewport.width() - size.width()) / 2,
                          (viewport.height() - size.height()) / 2), size);
}

void placeSleepButton(QPushButton *button, const QRectF &area, const QRect &position) {
    const qreal scale = area.width() / 520;
    button->setGeometry(QRectF(area.topLeft() + QPointF(position.topLeft()) * scale,
                               QSizeF(position.size()) * scale).toAlignedRect());
    button->setStyleSheet(QStringLiteral(
        "QPushButton { color: #4c301e; background: #e8c98f; border: %1px solid #fff1ba;"
        " font-size: %2px; padding: 0; }"
        "QPushButton:disabled { border-color: #8f6038; }"
        "QPushButton:hover { background: #f4dca6; }")
        .arg(qMax(1, qRound(2 * scale))).arg(qMax(1, qRound(13 * scale))));
}
}

SleepConfirmationOverlay::SleepConfirmationOverlay(QWidget *viewport)
    : CanvasOverlay(viewport),
      m_rest(new QPushButton(QStringLiteral("休息 [回车]"), this)),
      m_cancel(new QPushButton(QStringLiteral("取消 [Esc]"), this)) {
    setObjectName(QStringLiteral("sleepConfirmation"));
    setAccessibleName(QStringLiteral("休息确认"));
    setAttribute(Qt::WA_NoMousePropagation);
    setFocusPolicy(Qt::StrongFocus);
    m_rest->setObjectName(QStringLiteral("confirmSleep"));
    m_cancel->setObjectName(QStringLiteral("cancelSleep"));
    for (auto *button : {m_rest, m_cancel}) {
        button->setAutoRepeat(false);
        button->installEventFilter(this);
    }
    connect(m_rest, &QPushButton::clicked, this, &SleepConfirmationOverlay::restRequested);
    connect(m_cancel, &QPushButton::clicked, this, &SleepConfirmationOverlay::cancelRequested);
    hide();
}

void SleepConfirmationOverlay::present(const QString &wakeDate) {
    m_wakeDate = wakeDate;
    setAccessibleDescription(QStringLiteral("要休息到 %1 吗？选择期间时间暂停。").arg(wakeDate));
    show();
    raise();
    m_rest->setFocus(Qt::OtherFocusReason);
    update();
}

void SleepConfirmationOverlay::resizeEvent(QResizeEvent *event) {
    CanvasOverlay::resizeEvent(event);
    const QRectF area = sleepPageRect(size());
    placeSleepButton(m_rest, area, QRect(157, 182, 206, 28));
    placeSleepButton(m_cancel, area, QRect(193, 220, 134, 24));
}

void SleepConfirmationOverlay::keyPressEvent(QKeyEvent *event) {
    if (!event->isAutoRepeat()) {
        if (event->key() == Qt::Key_Escape) m_cancel->click();
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) m_rest->click();
    }
    event->accept();
}

bool SleepConfirmationOverlay::eventFilter(QObject *watched, QEvent *event) {
    if ((watched == m_rest || watched == m_cancel) &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->isAutoRepeat()) return true;
        if (event->type() == QEvent::KeyPress) {
            if (key->key() == Qt::Key_Escape) {
                m_cancel->click();
                return true;
            }
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                static_cast<QPushButton *>(watched)->click();
                return true;
            }
        }
    }
    return CanvasOverlay::eventFilter(watched, event);
}

void SleepConfirmationOverlay::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(5, 9, 24, 180));
    const QRectF area = sleepPageRect(size());
    const qreal scale = area.width() / 520;
    painter.translate(area.topLeft());
    painter.scale(scale, scale);
    painter.fillRect(QRect(64, 64, 392, 194), QColor("#8f6038"));
    painter.fillRect(QRect(68, 68, 384, 186), QColor("#e8c98f"));
    painter.setPen(QColor("#4c301e"));
    QFont textFont = font();
    textFont.setPixelSize(15);
    painter.setFont(textFont);
    painter.drawText(QRect(80, 85, 360, 25), Qt::AlignCenter, QStringLiteral("要休息到早晨吗？"));
    textFont.setPixelSize(13);
    painter.setFont(textFont);
    painter.drawText(QRect(80, 120, 360, 25), Qt::AlignCenter, m_wakeDate);
    painter.drawText(QRect(80, 151, 360, 20), Qt::AlignCenter, QStringLiteral("选择期间时间暂停"));
}

WakeSummaryPage::WakeSummaryPage(QWidget *parent)
    : QWidget(parent), m_continue(new QPushButton(QStringLiteral("醒来 [回车]"), this)) {
    setObjectName(QStringLiteral("wakeSummary"));
    m_continue->setObjectName(QStringLiteral("continueMorning"));
    m_continue->setAutoRepeat(false);
    m_continue->installEventFilter(this);
    connect(m_continue, &QPushButton::clicked, this, &WakeSummaryPage::continueRequested);
}

void WakeSummaryPage::focusContinue() {
    m_continue->setFocus(Qt::OtherFocusReason);
}

void WakeSummaryPage::present(const QString &title, const QString &date, const QString &dateChange,
                              const QStringList &rows, const QString &saveStatus,
                              bool saved, bool canContinue) {
    m_title = title;
    m_date = date;
    m_dateChange = dateChange;
    m_rows = rows;
    m_saveStatus = saveStatus;
    m_saved = saved;
    m_continue->setEnabled(canContinue);
    setAccessibleDescription(QStringList{title, date, dateChange}.join(u'\n') +
                             u'\n' + rows.join(u'\n') + u'\n' + saveStatus);
    update();
}

void WakeSummaryPage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    placeSleepButton(m_continue, sleepPageRect(size()), QRect(157, 270, 206, 28));
}

bool WakeSummaryPage::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_continue &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->isAutoRepeat()) return true;
        if (event->type() == QEvent::KeyPress &&
            (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter || key->key() == Qt::Key_Escape)) {
            m_continue->click();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void WakeSummaryPage::paintEvent(QPaintEvent *) {
    const QColor nightSky("#101b37");
    const QColor moonlight("#fff0ba");
    QPainter painter(this);
    painter.fillRect(rect(), nightSky);
    const QRectF area = sleepPageRect(size());
    const qreal scale = area.width() / 520;
    painter.translate(area.topLeft());
    painter.scale(scale, scale);

    for (int i = 0; i < 34; ++i) {
        painter.fillRect((i * 83 + 17) % 510, (i * 47 + 9) % 310,
                         i % 3 == 0 ? 2 : 1, 2, QColor("#b6c6dc"));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(moonlight);
    painter.drawEllipse(QRect(420, 23, 28, 28));
    painter.setBrush(nightSky);
    painter.drawEllipse(QRect(430, 18, 26, 26));
    QFont textFont = font();
    textFont.setPixelSize(13);
    painter.setFont(textFont);
    painter.setPen(moonlight);
    painter.drawText(QRect(70, 18, 380, 23), Qt::AlignCenter, m_title);
    painter.fillRect(QRect(64, 49, 392, 161), QColor("#8f6038"));
    painter.fillRect(QRect(68, 53, 384, 153), QColor("#e8c98f"));
    painter.setPen(QColor("#4c301e"));
    painter.drawText(QRect(80, 62, 360, 23), Qt::AlignCenter, m_date);
    painter.drawText(QRect(80, 89, 360, 18), Qt::AlignCenter, m_dateChange);
    for (int i = 0; i < m_rows.size(); ++i)
        painter.drawText(QRect(94, 117 + 25 * i, 335, 21), Qt::AlignVCenter, m_rows.at(i));
    painter.setPen(m_saved ? QColor("#b7e4a5") : QColor("#ffbc9b"));
    painter.drawText(QRect(60, 211, 400, 23), Qt::AlignCenter, m_saveStatus);
}
