#ifndef FARMHUD_H
#define FARMHUD_H

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QRegion>
#include <QWidget>
#include <QtMath>

// Read-only presentation snapshots. GameClock and EconomyService remain the owners.
class FarmHud final : public QWidget {
public:
    explicit FarmHud(QWidget *canvas) : QWidget(canvas),
        m_date(new QLabel(this)), m_time(new QLabel(this)),
        m_money(new QLabel(this)), m_water(new QLabel(this)),
        m_frame(QPixmap(QStringLiteral(":/farm-ui/slots.png")).copy(2, 98, 44, 44)),
        m_coin(QPixmap(QStringLiteral(":/farm-ui/coins.png")).copy(0, 16, 32, 32)) {
        setObjectName(QStringLiteral("farmHud"));
        setAccessibleName(QStringLiteral("日期、时间、金币与水壶水量"));
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_NoMousePropagation);
        setFixedSize(168, 134);
        // Leave the map clickable in the gap between the two visible panels.
        setMask(QRegion(clockFrame()) + QRegion(moneyFrame()) + QRegion(waterFrame()));
        setStyleSheet(QStringLiteral("QLabel { color: #563b2b; background: transparent; }"));
        m_date->setObjectName(QStringLiteral("gameDate"));
        m_time->setObjectName(QStringLiteral("gameTime"));
        m_money->setObjectName(QStringLiteral("gameMoney"));
        m_water->setObjectName(QStringLiteral("gameWater"));
        m_date->setGeometry(9, 8, 150, 18);
        m_time->setGeometry(38, 33, 116, 28);
        m_money->setGeometry(54, 83, 104, 18);
        m_water->setGeometry(31, 114, 127, 16);
        m_date->setAlignment(Qt::AlignCenter);
        m_time->setAlignment(Qt::AlignCenter);
        m_money->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_water->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_time->setAccessibleName(QStringLiteral("时间"));
        m_money->setAccessibleName(QStringLiteral("金币"));
        m_water->setAccessibleName(QStringLiteral("水壶剩余水量"));
        for (auto *label : {m_date, m_time, m_money, m_water}) {
            label->setAttribute(Qt::WA_TransparentForMouseEvents);
            QFont font = label->font();
            font.setBold(label == m_time || label == m_money);
            label->setFont(font);
        }
        canvas->installEventFilter(this);
        anchor();
    }

    void setDate(const QString &date) {
        m_date->setText(date);
        fit(m_date, 11);
    }
    void setTime(int hour, int minute) {
        m_hour = hour;
        m_minute = minute;
        m_time->setText(QStringLiteral("%1:%2").arg(hour, 2, 10, QLatin1Char('0'))
                           .arg(minute, 2, 10, QLatin1Char('0')));
        fit(m_time, 22);
        update();
    }
    void setMoney(int coins) {
        m_money->setText(QLocale(QLocale::English).toString(coins));
        fit(m_money, 13);
    }
    void setWater(int remaining, int total) {
        m_water->setText(QStringLiteral("水壶 %1 / %2").arg(remaining).arg(total));
        fit(m_water, 12);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == parentWidget() && event->type() == QEvent::Resize) anchor();
        return QWidget::eventFilter(watched, event);
    }
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        drawFrame(painter, clockFrame());
        drawFrame(painter, moneyFrame());
        drawFrame(painter, waterFrame());
        painter.drawPixmap(QRect(31, 82, 20, 20), m_coin);
        painter.setPen(QPen(QColor("#b48a60"), 1));
        painter.drawLine(12, 29, 155, 29);
        const QPointF centre(23, 47);
        painter.setPen(QPen(QColor("#79543b"), 1));
        painter.setBrush(QColor("#fff1cc"));
        painter.drawEllipse(centre, 10, 10);
        for (int i = 0; i < 4; ++i) {
            const qreal a = qDegreesToRadians(qreal(i * 90));
            painter.drawLine(centre + QPointF(qSin(a) * 8, -qCos(a) * 8),
                             centre + QPointF(qSin(a) * 9, -qCos(a) * 9));
        }
        const auto hand = [&painter, centre](qreal degrees, qreal length) {
            const qreal angle = qDegreesToRadians(degrees);
            painter.drawLine(centre, centre + QPointF(qSin(angle) * length, -qCos(angle) * length));
        };
        painter.setPen(QPen(QColor("#563b2b"), 1.5));
        hand((m_hour % 12 + m_minute / 60.0) * 30, 5);
        hand(m_minute * 6, 7);
    }

private:
    static QRect clockFrame() { return QRect(0, 0, 168, 74); }
    static QRect moneyFrame() { return QRect(24, 78, 144, 28); }
    static QRect waterFrame() { return QRect(24, 110, 144, 24); }
    void anchor() { move(qMax(0, parentWidget()->width() - width() - 8), 8); }
    static void fit(QLabel *label, int largest) {
        QFont font = label->font();
        for (int size = largest; size >= 7; --size) {
            font.setPixelSize(size);
            if (QFontMetrics(font).horizontalAdvance(label->text()) <= label->width() - 2) break;
        }
        label->setFont(font);
    }
    void drawFrame(QPainter &painter, const QRect &target) const {
        // Nine-slice the existing inventory frame, preserving pixel-sized corners.
        constexpr int Edge = 7;
        const int sx[] = {0, Edge, 44 - Edge, 44};
        const int tx[] = {target.x(), target.x() + Edge, target.x() + target.width() - Edge,
                          target.x() + target.width()};
        const int ty[] = {target.y(), target.y() + Edge, target.y() + target.height() - Edge,
                          target.y() + target.height()};
        for (int y = 0; y < 3; ++y) for (int x = 0; x < 3; ++x)
            painter.drawPixmap(QRect(tx[x], ty[y], tx[x+1]-tx[x], ty[y+1]-ty[y]), m_frame,
                               QRect(sx[x], sx[y], sx[x+1]-sx[x], sx[y+1]-sx[y]));
    }
    QLabel *m_date;
    QLabel *m_time;
    QLabel *m_money;
    QLabel *m_water;
    QPixmap m_frame;
    QPixmap m_coin;
    int m_hour = 6;
    int m_minute = 0;
};

#endif // FARMHUD_H
