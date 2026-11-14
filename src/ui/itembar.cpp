#include "itembar.h"
#include "farmicons.h"
#include "economy/economyservice.h"
#include "map/mapwidget.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

namespace {
constexpr int SlotSize = 32;
constexpr int CaptionHeight = 16;
}

ItemBar::ItemBar(QWidget *parent)
    : QWidget(parent) {
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("农具栏"));
    setAccessibleDescription(toolName(m_selected));
    const QPixmap tools(QStringLiteral(":/tilesets/sprout/Basic_tools_and_meterials.png"));
    m_icons[static_cast<int>(ToolType::EmptyHand)] = QPixmap(QStringLiteral(":/ui/sprout/Catpaw Mouse icon.png"));
    m_icons[static_cast<int>(ToolType::Hoe)] = tools.copy(32, 0, 16, 16);
    m_icons[static_cast<int>(ToolType::WateringCan)] = tools.copy(0, 0, 16, 16);
    for (CropType crop : CropCatalog::cropTypes())
        m_icons[static_cast<int>(toolForSeed(crop))] = FarmIcons::crop(CropCatalog::definition(crop).seedIcon);
    m_icons[static_cast<int>(ToolType::Fertilizer)] = FarmIcons::fertilizer();

    if (parent)
        parent->installEventFilter(this);
}

QSize ItemBar::sizeHint() const {
    return QSize(232, 64 + CaptionHeight);
}

void ItemBar::setStock(const EconomyService &economy) {
    m_counts.clear();
    for (CropType crop : CropCatalog::cropTypes())
        m_counts.insert(static_cast<int>(toolForSeed(crop)), economy.seedCount(crop));
    m_counts.insert(static_cast<int>(ToolType::Fertilizer), economy.fertilizerCount());
    if (m_counts.contains(static_cast<int>(m_selected)) && m_counts.value(static_cast<int>(m_selected)) == 0)
        selectTool(ToolType::EmptyHand);
    update();
}

QRect ItemBar::slotRect(int index) const {
    const int side = qMin(SlotSize, qMin(width() / ToolbarSlotCount, qMax(0, height() - CaptionHeight)));
    const int left = (width() - ToolbarSlotCount * side) / 2;
    return QRect(left + side * index, height() - 40, side, side);
}

void ItemBar::selectTool(ToolType tool) {
    if (m_counts.contains(static_cast<int>(tool)) && m_counts.value(static_cast<int>(tool)) == 0)
        tool = ToolType::EmptyHand;
    if ((tool != ToolType::EmptyHand && toolSlot(tool) < 0) || m_selected == tool)
        return;
    m_selected = tool;
    setAccessibleDescription(toolName(tool));
    update();
    emit itemSelected(static_cast<int>(tool));
}

bool ItemBar::acceptsPlayerInput() const {
    const auto *map = qobject_cast<const MapWidget *>(parentWidget());
    return isEnabled() && (!map || !map->isInputBlocked());
}

bool ItemBar::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parentWidget() && event->type() == QEvent::KeyPress) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_0 && key->modifiers() == Qt::NoModifier) {
            if (acceptsPlayerInput() && !key->isAutoRepeat()) selectTool(ToolType::EmptyHand);
            return true;
        }
    }
    if (watched == parentWidget() && event->type() == QEvent::Wheel) {
        wheelEvent(static_cast<QWheelEvent *>(event));
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ItemBar::wheelEvent(QWheelEvent *event) {
    event->accept();
    if (!acceptsPlayerInput()) {
        m_wheelRemainder = 0;
        return;
    }
    if (event->phase() == Qt::ScrollBegin)
        m_wheelRemainder = 0;
    // A conventional notch is 120 angle units. Some touchpads send only pixels.
    const int angle = event->angleDelta().y();
    m_wheelRemainder += angle ? angle : event->pixelDelta().y() * 4;
    const int steps = m_wheelRemainder / 120;
    m_wheelRemainder %= 120;
    if (steps) {
        const int current = m_selected == ToolType::EmptyHand ? (steps < 0 ? -1 : 0) : toolSlot(m_selected);
        const int next = (current - steps % ToolbarSlotCount + ToolbarSlotCount) % ToolbarSlotCount;
        selectTool(PlayableTools[next]);
    }
    if (event->phase() == Qt::ScrollEnd)
        m_wheelRemainder = 0;
}

void ItemBar::mousePressEvent(QMouseEvent *event) {
    event->accept();
    if (!acceptsPlayerInput() || event->button() != Qt::LeftButton)
        return;
    const QPoint pos(qFloor(event->position().x()), qFloor(event->position().y()));
    for (int i = 0; i < ToolbarSlotCount; ++i) {
        if (slotRect(i).contains(pos)) {
            selectTool(m_selected == PlayableTools[i] ? ToolType::EmptyHand : PlayableTools[i]);
            return;
        }
    }
}

void ItemBar::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawPixmap(QRect((width()-232)/2, height()-64, 232, 64),
                       QPixmap(QStringLiteral(":/farm-ui/hotbar.png")));
    for (int i = 0; i < ToolbarSlotCount; ++i) {
        const QRect slot = slotRect(i);
        const ToolType tool = PlayableTools[i];
        const bool selected = m_selected == tool;
        if (selected) {
            painter.setPen(QColor(81, 53, 43));
            painter.drawRect(slot.adjusted(0, 0, -1, -1));
            painter.setPen(QColor(255, 229, 148));
            painter.drawRect(slot.adjusted(1, 1, -2, -2));
        }
        const QPixmap &icon = m_icons[static_cast<int>(tool)];
        const bool hasItem = !m_counts.contains(static_cast<int>(tool)) || m_counts.value(static_cast<int>(tool)) > 0;
        if (hasItem) painter.drawPixmap(slot.x() + (slot.width() - icon.width()) / 2,
                           slot.y() + (slot.height() - icon.height()) / 2, icon);
        if (hasItem && m_counts.contains(static_cast<int>(tool))) {
            QFont number = font(); number.setPixelSize(9); number.setBold(true); painter.setFont(number);
            painter.setPen(QColor(75, 43, 25));
            painter.drawText(slot.adjusted(1, 0, -3, -2), Qt::AlignRight | Qt::AlignBottom,
                             QString::number(m_counts.value(static_cast<int>(tool))));
        }
    }

    QFont caption = font();
    caption.setPixelSize(11);
    painter.setFont(caption);
    const QString label = toolName(m_selected);
    const int labelWidth = painter.fontMetrics().horizontalAdvance(label) + 14;
    const QRect plaque((width() - labelWidth) / 2, 0,
                       labelWidth, CaptionHeight);
    painter.fillRect(plaque, QColor(77, 48, 38));
    painter.setPen(QColor(255, 235, 184));
    painter.drawText(plaque, Qt::AlignCenter, label);
}
