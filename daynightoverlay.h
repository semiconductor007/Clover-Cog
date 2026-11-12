#ifndef DAYNIGHTOVERLAY_H
#define DAYNIGHTOVERLAY_H

#include <QWidget>
#include <QColor>
#include "gamecalendar.h"

class DayNightOverlay : public QWidget {
public:
    explicit DayNightOverlay(QWidget *parent);
    void setGameTime(qint64 gameMinutes, bool outdoors = true, Season season = Season::Spring);
    QColor tint() const { return m_tint; }
protected:
    void paintEvent(QPaintEvent *) override;
private:
    QColor m_tint{0, 0, 0, 0};
};

#endif
