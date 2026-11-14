#ifndef ITEMBAR_H
#define ITEMBAR_H

#include "player/tooltype.h"
#include <QWidget>
#include <array>
#include <QHash>

class EconomyService;

class ItemBar : public QWidget {
    Q_OBJECT
public:
    explicit ItemBar(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    ToolType selectedTool() const { return m_selected; }
    void selectTool(ToolType tool);
    void setStock(const EconomyService &economy);
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
    QHash<int, int> m_counts;
    std::array<QPixmap, ToolCount> m_icons;
};

#endif // ITEMBAR_H
