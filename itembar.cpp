//itembar.cpp
#include "itembar.h"
#include <QWheelEvent>
#include <QPainter>
#include <QDebug>

ItemBar::ItemBar(QWidget *parent) : QWidget(parent) {
    // 初始化显示控件
    m_itemDisplay = new QLabel(this);
    m_itemDisplay->setFixedSize(221, 54); // 确保尺寸与图片一致
    updateItemDisplay();

}



void ItemBar::wheelEvent(QWheelEvent *event) {
    int delta = event->angleDelta().y();
    if (delta > 0) {
        m_currentIndex = (m_currentIndex - 1 + 6) % 6; // 向上滚动减1
    } else {
        m_currentIndex = (m_currentIndex + 1) % 6; // 向下滚动加1
    }
    updateItemDisplay();
    emit itemSelected(m_currentIndex); // 仅传递索引
}

void ItemBar::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    // 绘制背景（可选）
    QPixmap background(":/ui/itembar/itembar2.png");
    if (!background.isNull()) {
        painter.drawPixmap(0, 0, width(), height(), background);
    }
}

void ItemBar::updateItemDisplay() {
    QString path = QString(":/ui/itembar/itembar%1.png").arg(m_currentIndex);
    QPixmap pixmap(path);

    if (pixmap.isNull()) {
        qCritical() << "图片加载失败:" << path;
        return;
    }

    if (m_itemDisplay) {
        m_itemDisplay->setPixmap(pixmap.scaled(m_itemDisplay->size(), Qt::KeepAspectRatio));
    }
}
