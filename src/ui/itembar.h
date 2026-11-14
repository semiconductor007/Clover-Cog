#ifndef ITEMBAR_H
#define ITEMBAR_H

#include "player/tooltype.h"
#include <QWidget>
#include <array>
#include <QHash>
#include "slotgrid.h"

class EconomyService;

class ItemBar : public QWidget {
    Q_OBJECT
public:
    explicit ItemBar(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    ToolType selectedTool() const { return m_selected; }
    int selectedSlot() const { return m_activeSlot; }
    void selectSlot(int slot);
    void selectTool(ToolType tool);
    void setStock(const EconomyService &economy);
    void wheelEvent(QWheelEvent *event) override;

signals:
    void itemSelected(int index);
    void itemEquipped(int slot, bool equipped);
    void slotSelected(int slot);
    void stackMoveRequested(StackMove move);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QRect slotRect(int index) const;
    bool acceptsPlayerInput() const;
    ToolType m_selected = ToolType::EmptyHand;
    int m_wheelRemainder = 0;
    QHash<int, int> m_counts;
    std::array<QPixmap, ToolCount> m_icons;
    QVector<InventoryStack> m_cells;
    quint64 m_revision=0;
    int m_activeSlot=-1, m_pressSlot=-1;
    QPoint m_press;
    bool m_dragging=false;
    StackMove m_drag;
    SlotDropSurface m_dropSurface;
};

#endif // ITEMBAR_H
