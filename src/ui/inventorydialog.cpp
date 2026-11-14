#include "inventorydialog.h"
#include "slotgrid.h"
#include <QResizeEvent>

InventoryDialog::InventoryDialog(QWidget *parent)
    : FarmPanel(QStringLiteral("背包"),parent),m_grid(new SlotGrid(false,this)),m_date(new QLabel(this)) {
    setObjectName(QStringLiteral("inventoryDialog"));
    m_date->setObjectName(QStringLiteral("bagDate")); m_date->hide();
    m_grid->setObjectName(QStringLiteral("bagSlots"));
    m_grid->setAccessibleName(QStringLiteral("背包格位；首排为物品栏"));
    body->addWidget(m_grid);
    addFooter(QString());
    for (auto *label : findChildren<QLabel *>()) label->hide();
    auto *close=findChild<QPushButton *>(QStringLiteral("closePanel"));
    body->itemAt(body->count()-1)->layout()->removeWidget(close);
    close->setText(QStringLiteral("×")); close->setFixedSize(28,28);
    close->setAccessibleName(QStringLiteral("关闭背包"));
    close->setToolTip(QStringLiteral("关闭背包（E / Tab / Esc）"));
    body->setContentsMargins(12,12,12,12); body->setSpacing(0);
    m_grid->moveRequested=[this](StackMove move){emit stackMoveRequested(move);};
    m_grid->slotChosen=[this](int slot) {
        if (slot<0 || slot>=7 || slot>=m_grid->cells().size() || !m_grid->cells()[slot].item) return;
        emit slotRequested(slot);
        emit toolRequested(StackItem::tool(m_grid->cells()[slot].item));
        accept();
    };
    setFixedSize(546,312); m_grid->setFocus();
}

void InventoryDialog::setState(const EconomyService &economy, ToolType selected, int day, int active) {
    if (active==-2) {
        active=-1;
        const int item=StackItem::fromTool(selected);
        for (int i=0; i<7; ++i) if (item && economy.bagSlots()[i].item==item) {active=i;break;}
    }
    m_grid->setState(economy.bagSlots(),economy.revision(),active);
    const auto date=SeasonCalendar::dateForDay(day);
    m_date->setText(QStringLiteral("第 %1 年 · %2 · 第 %3 天").arg(date.year)
        .arg(SeasonCalendar::seasonName(date.season)).arg(date.dayOfSeason));
    summary->setText(QStringLiteral("金币 %1 · 农产品 %2 / %3").arg(economy.coins())
        .arg(economy.inventory().usedCapacity()).arg(economy.inventory().capacity()));
}

void InventoryDialog::paintEvent(QPaintEvent *) {
    QPainter p(this); p.fillRect(rect(),QColor("#e8cfa6"));
    p.setPen(QPen(QColor("#90625d"),4)); p.drawRect(rect().adjusted(2,2,-3,-3));
}

void InventoryDialog::resizeEvent(QResizeEvent *event) {
    FarmPanel::resizeEvent(event);
    if (auto *close=findChild<QPushButton *>(QStringLiteral("closePanel"))) {
        close->move(width()-close->width()-12,12); close->raise();
    }
}

bool InventoryDialog::eventFilter(QObject *object,QEvent *event) {
    if (event->type()==QEvent::KeyPress) {
        const auto *key=static_cast<QKeyEvent *>(event);
        if ((key->key()==Qt::Key_E || key->key()==Qt::Key_Tab) && key->modifiers()==Qt::NoModifier) {
            if (!key->isAutoRepeat()) {m_grid->cancelDrag();reject();} return true;
        }
    }
    return FarmPanel::eventFilter(object,event);
}
