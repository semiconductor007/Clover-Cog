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
constexpr int CaptionHeight = 0;
}

ItemBar::ItemBar(QWidget *parent)
    : QWidget(parent),m_dropSurface(this,false,[this](QPoint p) {
        for (int i=0;i<7;++i) if(slotRect(i).contains(p)) return i;
        return -1;
    }) {
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("农具栏"));
    setAccessibleDescription(toolName(m_selected));
    setToolTip(toolName(m_selected));
    m_cells.resize(21);
    for (int i=0;i<7;++i) m_cells[i]={StackItem::fromTool(PlayableTools[i]),1};
    m_dropSurface.drop=[this](StackMove move){emit stackMoveRequested(move);};
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
    m_cells=economy.bagSlots(); m_revision=economy.revision();
    selectSlot(m_activeSlot); update();
}

QRect ItemBar::slotRect(int index) const {
    const int side = qMin(SlotSize, qMin(width() / ToolbarSlotCount, qMax(0, height() - CaptionHeight)));
    const int left = (width() - ToolbarSlotCount * side) / 2;
    return QRect(left + side * index, height() - 40, side, side);
}

void ItemBar::selectTool(ToolType tool) {
    if (tool==ToolType::Axe) return;
    const int item=StackItem::fromTool(tool);
    if (tool!=ToolType::EmptyHand && !item) return;
    int index=-1;
    for (int i=0;item && i<7 && i<m_cells.size();++i) if(m_cells[i].item==item){index=i;break;}
    selectSlot(index);
}

void ItemBar::selectSlot(int index) {
    if (index<0 || index>=7 || index>=m_cells.size() || !m_cells[index].item) index=-1;
    const ToolType tool=index<0 ? ToolType::EmptyHand : StackItem::tool(m_cells[index].item);
    const bool changed=m_selected!=tool;
    const bool slotChanged=m_activeSlot!=index;
    m_activeSlot=index; m_selected=tool;
    const QString name=index<0 ? toolName(ToolType::EmptyHand) : FarmIcons::stackName(m_cells[index]);
    setAccessibleDescription(toolName(tool)); setToolTip(name);
    update();
    if (slotChanged) emit slotSelected(index);
    if (changed) emit itemSelected(static_cast<int>(tool));
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
        const int current = m_activeSlot<0 ? (steps < 0 ? -1 : 0) : m_activeSlot;
        const int next = (current - steps % ToolbarSlotCount + ToolbarSlotCount) % ToolbarSlotCount;
        selectSlot(next);
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
            m_pressSlot=i; m_press=pos; m_dragging=false;
            m_drag={}; m_drag.source=i; m_drag.revision=m_revision;
            if (i<m_cells.size()) {m_drag.expected=m_cells[i];m_drag.quantity=m_cells[i].quantity;}
            return;
        }
    }
}

void ItemBar::mouseMoveEvent(QMouseEvent *event) {
    if (m_pressSlot>=0 && (event->position().toPoint()-m_press).manhattanLength()>=QApplication::startDragDistance())
        m_dragging=true;
}

void ItemBar::changeEvent(QEvent *event) {
    if (event->type()==QEvent::EnabledChange && !isEnabled()) {m_pressSlot=-1;m_dragging=false;}
    QWidget::changeEvent(event);
}

void ItemBar::mouseReleaseEvent(QMouseEvent *event) {
    if (m_pressSlot<0) return;
    const int index=m_pressSlot; m_pressSlot=-1;
    if (!acceptsPlayerInput()) return;
    if (m_dragging) SlotDropSurface::deliver(this,event->globalPosition().toPoint(),m_drag);
    else if (slotRect(index).contains(event->position().toPoint())) selectSlot(m_activeSlot==index ? -1 : index);
    m_dragging=false;
}

void ItemBar::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawPixmap(QRect((width()-232)/2, height()-64, 232, 64),
                       FarmIcons::hotbarBackground(m_activeSlot));
    for (int i = 0; i < ToolbarSlotCount; ++i) {
        const QRectF slot = FarmIcons::hotbarCell(QRectF((width()-232)/2, height()-64,232,64),i);
        const auto stack=i<m_cells.size() ? m_cells[i] : InventoryStack{};
        const QPixmap icon=FarmIcons::stackIcon(stack.item);
        const bool hasItem=stack.item!=0;
        if (hasItem) FarmIcons::centeredIcon(painter,icon,slot,qMax(icon.width(),icon.height()));
        if (hasItem && !StackItem::isTool(stack.item)) {
            QFont number = font(); number.setPixelSize(9); number.setBold(true); painter.setFont(number);
            painter.setPen(QColor(75, 43, 25));
            painter.drawText(slot.adjusted(1, 0, -3, -2), Qt::AlignRight | Qt::AlignBottom,
                             QString::number(stack.quantity));
        }
    }

}
