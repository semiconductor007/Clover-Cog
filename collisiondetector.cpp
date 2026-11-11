#include "collisiondetector.h"

CollisionDetector::CollisionDetector(const QVector<QVector<int>> *collisionLayer,
                                     int tileWidth, int tileHeight)
    : m_collisionLayer(collisionLayer), m_tileWidth(tileWidth), m_tileHeight(tileHeight) {}

bool CollisionDetector::isColliding(const QRect &characterRect) const {
    if (!m_collisionLayer || m_tileWidth <= 0 || m_tileHeight <= 0 ||
        characterRect.isEmpty() || characterRect.right() < 0 || characterRect.bottom() < 0)
        return false;

    const auto &layer = *m_collisionLayer;
    // QRect's right and bottom edges are inclusive.
    const int left = qMax(0, characterRect.left() / m_tileWidth);
    const int right = characterRect.right() / m_tileWidth;
    const int top = qMax(0, characterRect.top() / m_tileHeight);
    const int bottom = qMin(layer.size() - 1, characterRect.bottom() / m_tileHeight);
    for (int y = top; y <= bottom; ++y) {
        const auto &row = layer.at(y);
        const int last = qMin(row.size() - 1, right);
        for (int x = left; x <= last; ++x) {
            if (row.at(x) != 0) return true;
        }
    }
    return false;
}
