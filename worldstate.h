#ifndef WORLDSTATE_H
#define WORLDSTATE_H

#include "gameconstants.h"
#include "mapdata.h"

#include <QHash>
#include <QPoint>
#include <QSet>
#include <QVector>

enum class CropType {
    None,
    Wheat,
    Potato
};

class WorldState {
public:
    void initialize(const MapData &mapData);
    bool isValidTile(const QPoint &tilePos) const;

    const QVector<QVector<int>> &cultivatableLayer() const { return m_cultivatableLayer; }
    const QVector<QVector<int>> &cultivatedLayer() const { return m_cultivatedLayer; }
    const QVector<QVector<int>> &cropLayer() const { return m_cropLayer; }
    const QVector<QVector<int>> &treeLayer() const { return m_treeLayer; }
    const QVector<QVector<int>> &collisionLayer() const { return m_collisionLayer; }

    bool hasTreeAt(const QPoint &tilePos) const;
    bool damageTree(const QPoint &tilePos, int damage);
    bool isTileCultivatable(const QPoint &tilePos) const;
    bool cultivate(const QPoint &tilePos);
    bool isPlantable(const QPoint &tilePos) const;
    bool plantSeed(const QPoint &tilePos, CropType cropType, qint64 gameMinute);
    CropType harvest(const QPoint &tilePos);
    bool advanceGrowth(qint64 gameMinute);
    int cropGidAt(const QPoint &tilePos) const;
    int treeGidAt(const QPoint &tilePos) const;

private:
    struct TreeState {
        int health = 0;
    };

    int m_mapWidth = 0;
    int m_mapHeight = 0;
    QVector<QVector<int>> m_cultivatableLayer;
    QVector<QVector<int>> m_cultivatedLayer;
    QVector<QVector<int>> m_cropLayer;
    QVector<QVector<int>> m_treeLayer;
    QVector<QVector<int>> m_collisionLayer;
    QHash<QPoint, TreeState> m_trees;
    QHash<QPoint, qint64> m_plantingTimes;
    QHash<QPoint, CropType> m_plantedCropTypes;
    const QSet<int> m_plantableGids = {
        123, 124, 125, 134, GameConstants::CultivatedTileGid, 136, 145, 146, 147
    };
};

#endif // WORLDSTATE_H
