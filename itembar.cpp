#include "itembar.h"
#include "farmicons.h"
#include "mapwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

namespace {
constexpr int SlotSize = 32;
constexpr int CaptionHeight = 16;
}

ItemBar::ItemBar(QWidget *parent)
    : QWidget(parent), m_slots(QStringLiteral(":/farm-ui/slots.png")) {
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("农具栏"));
    setAccessibleDescription(toolName(m_selected));
    const QPixmap tools(QStringLiteral(":/tilesets/sprout/Basic_tools_and_meterials.png"));
    m_icons[static_cast<int>(ToolType::EmptyHand)] = QPixmap(QStringLiteral(":/ui/sprout/Catpaw Mouse icon.png"));
    m_icons[static_cast<int>(ToolType::Axe)] = tools.copy(16, 0, 16, 16);
    m_icons[static_cast<int>(ToolType::Hoe)] = tools.copy(32, 0, 16, 16);
    m_icons[static_cast<int>(ToolType::WateringCan)] = tools.copy(0, 0, 16, 16);
    for (CropType crop : CropCatalog::cropTypes())
        m_icons[static_cast<int>(toolForSeed(crop))] = FarmIcons::crop(CropCatalog::definition(crop).seedIcon);
    m_icons[static_cast<int>(ToolType::Fertilizer)] = FarmIcons::fertilizer();

    if (parent)
        parent->installEventFilter(this);
}

QSize ItemBar::sizeHint() const {
    return QSize(ToolCount * SlotSize, SlotSize + CaptionHeight);
}

QRect ItemBar::slotRect(int index) const {
    const int side = qMin(SlotSize, qMin(width() / ToolCount, qMax(0, height() - CaptionHeight)));
    const int left = (width() - ToolCount * side) / 2;
    return QRect(left + side * index, height() - side, side, side);
}

void ItemBar::selectTool(ToolType tool) {
    const int index = static_cast<int>(tool);
    if (index < 0 || index >= ToolCount || m_selected == tool)
        return;
    m_selected = tool;
    setAccessibleDescription(toolName(tool));
    update();
    emit itemSelected(index);
}

bool ItemBar::acceptsPlayerInput() const {
    const auto *map = qobject_cast<const MapWidget *>(parentWidget());
    return isEnabled() && (!map || !map->isInputBlocked());
}

bool ItemBar::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parentWidget() && event->type() == QEvent::Wheel) {
        wheelEvent(static_cast<QWheelEvent *>(event));
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ItemBar::wheelEvent(QWheelEvent *event) {
    event->accept();
    if (!acceptsPlayerInput() || event->phase() == Qt::ScrollBegin)
        m_wheelRemainder = 0;
    if (!acceptsPlayerInput())
        return;
    // A conventional notch is 120 angle units. Some touchpads send only pixels.
    const int angle = event->angleDelta().y();
    m_wheelRemainder += angle ? angle : event->pixelDelta().y() * 4;
    const int steps = m_wheelRemainder / 120;
    m_wheelRemainder %= 120;
    if (steps)
        selectTool(static_cast<ToolType>((static_cast<int>(m_selected) - steps % ToolCount + ToolCount) % ToolCount));
    if (event->phase() == Qt::ScrollEnd)
        m_wheelRemainder = 0;
}

void ItemBar::mousePressEvent(QMouseEvent *event) {
    event->accept();
    if (!acceptsPlayerInput() || event->button() != Qt::LeftButton)
        return;
    for (int i = 0; i < ToolCount; ++i) {
        if (QRectF(slotRect(i)).contains(event->position())) {
            selectTool(static_cast<ToolType>(i));
            return;
        }
    }
}

void ItemBar::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int i = 0; i < ToolCount; ++i) {
        const QRect slot = slotRect(i);
        const bool selected = i == static_cast<int>(m_selected);
        const QRect source(selected ? 57 : 9, 57, 30, 32);
        painter.drawPixmap(slot.adjusted(1, 1, -1, 0), m_slots, source);
        if (selected) {
            painter.setPen(QColor(81, 53, 43));
            painter.drawRect(slot.adjusted(0, 0, -1, -1));
            painter.setPen(QColor(255, 229, 148));
            painter.drawRect(slot.adjusted(1, 1, -2, -2));
        }
        const QPixmap &icon = m_icons[i];
        painter.drawPixmap(slot.x() + (slot.width() - icon.width()) / 2,
                           slot.y() + (slot.height() - icon.height()) / 2, icon);
    }

    QFont caption = font();
    caption.setPixelSize(11);
    painter.setFont(caption);
    const QString label = toolName(m_selected);
    const int labelWidth = painter.fontMetrics().horizontalAdvance(label) + 14;
    const QRect plaque((width() - labelWidth) / 2, slotRect(0).top() - CaptionHeight,
                       labelWidth, CaptionHeight);
    painter.fillRect(plaque, QColor(77, 48, 38));
    painter.setPen(QColor(255, 235, 184));
    painter.drawText(plaque, Qt::AlignCenter, label);
}
