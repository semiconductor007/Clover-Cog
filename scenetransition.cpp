#include "scenetransition.h"

#include <QEvent>
#include <QPainter>
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
const std::array<LightStop, 9> lightStops{{
    {0, night},
    {4 * 60, night},
    {6 * 60, QColor(250, 203, 150, 48)},
    {8 * 60, sunlight},
    {16 * 60 + 30, sunlight},
    {18 * 60 + 30, QColor(223, 139, 107, 58)},
    {20 * 60, QColor(79, 69, 112, 94)},
    {22 * 60, night},
    {24 * 60, night}
}};

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
    const int minuteOfDay = int(qMax<qint64>(0, minutes) % (24 * 60));
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
