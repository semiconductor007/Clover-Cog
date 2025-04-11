#include "collisiondetector.h"
#include<QDebug>

CollisionDetector::CollisionDetector(const QVector<QVector<int>>& collisionLayer, int tileWidth, int tileHeight)
    : m_collisionLayer(collisionLayer), m_tileWidth(tileWidth), m_tileHeight(tileHeight) {}

bool CollisionDetector::isColliding(const QRect &characterRect) const {
    if (m_tileWidth <= 0 || m_tileHeight <= 0 || m_collisionLayer.isEmpty()) {
        return false;
    }

    // 计算覆盖的图块范围
    int tileLeft = characterRect.left() / m_tileWidth;
    int tileRight = (characterRect.right() - 1) / m_tileWidth;
    int tileTop = characterRect.top() / m_tileHeight;
    int tileBottom = (characterRect.bottom() - 1) / m_tileHeight;

    // 边界约束
    tileLeft = qMax(0, tileLeft);
    tileRight = qMin(m_collisionLayer[0].size() - 1, tileRight);
    tileTop = qMax(0, tileTop);
    tileBottom = qMin(m_collisionLayer.size() - 1, tileBottom);

    // 遍历所有覆盖的图块
    for (int y = tileTop; y <= tileBottom; ++y) {
        for (int x = tileLeft; x <= tileRight; ++x) {
            if (m_collisionLayer[y][x] != 0) {
                return true; // 碰撞！
            }
        }
    }
    return false;
}
