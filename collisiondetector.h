#ifndef COLLISIONDETECTOR_H
#define COLLISIONDETECTOR_H

#include <QVector>
#include <QRect>

class CollisionDetector {
public:
    CollisionDetector(const QVector<QVector<int>> *collisionLayer,
                      int tileWidth, int tileHeight);
    bool isColliding(const QRect &characterRect) const;

private:
    const QVector<QVector<int>> *m_collisionLayer;
    int m_tileWidth;
    int m_tileHeight;
};

#endif // COLLISIONDETECTOR_H
