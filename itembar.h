#ifndef ITEMBAR_H
#define ITEMBAR_H

#include "tooltype.h"
#include <QWidget>
#include <array>

class ItemBar : public QWidget {
    Q_OBJECT
public:
    explicit ItemBar(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    ToolType selectedTool() const { return m_selected; }
    void selectTool(ToolType tool);
    void wheelEvent(QWheelEvent *event) override;

signals:
    void itemSelected(int index);
    void itemEquipped(int slot, bool equipped);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QRect slotRect(int index) const;
    bool acceptsPlayerInput() const;
    ToolType m_selected = ToolType::EmptyHand;
    int m_wheelRemainder = 0;
    QPixmap m_slots;
    std::array<QPixmap, ToolCount> m_icons;
};

#endif // ITEMBAR_H
