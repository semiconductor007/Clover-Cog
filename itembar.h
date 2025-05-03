//itembar.h
#ifndef ITEMBAR_H
#define ITEMBAR_H

#include <QWidget>
#include <QLabel>

class ItemBar : public QWidget {
    Q_OBJECT
public:
    explicit ItemBar(QWidget *parent = nullptr);
    ~ItemBar() = default; // 移除手动析构逻辑
     void wheelEvent(QWheelEvent *event) override;


protected:

    void paintEvent(QPaintEvent *event) override;

signals:
    void itemSelected(int index);
    void itemEquipped(int slot, bool equipped);

private:
    void updateItemDisplay();

    QLabel *m_itemDisplay = nullptr; // 直接显示物品栏整体图片
    int m_currentIndex = 0;           // 当前选中的物品索引（0~5）
};

#endif // ITEMBAR_H
