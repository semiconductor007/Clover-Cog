#include "inventorypanel.h"
#include "cropdefinition.h"
#include <QPainter>
#include <QPushButton>

InventoryPanel::InventoryPanel(QWidget *parent)
    : QWidget(parent), m_slots(QStringLiteral(":/inventory/slots.png")) {
    setGeometry(0, 0, 520, 320);
    setFocusPolicy(Qt::StrongFocus);
    QPushButton *close = new QPushButton(QStringLiteral("CLOSE [B / ESC]"), this);
    close->setGeometry(170, 262, 180, 25);
    close->setStyleSheet(QStringLiteral(
        "QPushButton { background:#e8c98f; color:#4c301e; border:2px solid #8f6038; }"));
    connect(close, &QPushButton::clicked, this, &InventoryPanel::closeRequested);
    hide();
}

void InventoryPanel::setState(const EconomyState &state) {
    m_snapshot = state;
    update();
}

void InventoryPanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 160));
    p.fillRect(QRect(42, 22, 436, 275), QColor("#8f6038"));
    p.fillRect(QRect(46, 26, 428, 267), QColor("#e8c98f"));
    p.setPen(QColor("#4c301e"));
    p.drawText(QRect(60, 34, 400, 24), Qt::AlignCenter, QStringLiteral("BACKPACK"));
    int used = 0;
    for (int count : m_snapshot.inventoryCounts) used += count;
    p.drawText(QRect(62, 60, 396, 22), Qt::AlignCenter,
               QStringLiteral("BAG %1/%2    COINS %3")
                   .arg(used).arg(m_snapshot.inventoryCapacity).arg(m_snapshot.coins));
    int y = 88;
    for (CropType type : CropCatalog::cropTypes()) {
        const CropDefinition &definition = CropCatalog::definition(type);
        for (bool seed : {false, true}) {
            const int quantity = (seed ? m_snapshot.seedCounts : m_snapshot.inventoryCounts)
                                     .value(static_cast<int>(type));
            const QRect slot(64, y, 32, 32);
            if (!m_slots.isNull()) p.drawPixmap(slot, m_slots, QRect(233, 121, 30, 32));
            else p.fillRect(slot, QColor("#c99f67"));
            p.setPen(QColor("#4c301e"));
            p.drawText(slot, Qt::AlignCenter, seed ? QStringLiteral("S") : definition.displayName.left(1));
            p.drawText(QRect(108, y, 225, 18), Qt::AlignVCenter,
                       definition.displayName + (seed ? QStringLiteral(" seeds") : QString()));
            p.drawText(QRect(108, y + 17, 285, 16), Qt::AlignVCenter,
                       seed ? QStringLiteral("Grow: %1 days / Buy: %2 coins")
                                  .arg(definition.growthDays).arg(definition.seedPrice)
                            : QStringLiteral("Sell: %1 coins each").arg(definition.sellPrice));
            p.drawText(QRect(388, y, 58, 32), Qt::AlignRight | Qt::AlignVCenter,
                       QStringLiteral("x%1").arg(quantity));
            y += 38;
        }
    }
    p.drawText(QRect(62, 242, 396, 18), Qt::AlignCenter,
               QStringLiteral("Seeds use no bag space. Sell crops at SELL [E]."));
}
