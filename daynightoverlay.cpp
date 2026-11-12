#include "daynightoverlay.h"
#include "daynightenvironment.h"

#include <QPainter>

DayNightOverlay::DayNightOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
}

void DayNightOverlay::setGameTime(qint64 gameMinutes, bool outdoors, Season season) {
    const QColor tint = DayNightEnvironment::tintAt(gameMinutes, outdoors, season);
    if (tint == m_tint) return;
    m_tint = tint;
    update();
}

void DayNightOverlay::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), m_tint);
}
