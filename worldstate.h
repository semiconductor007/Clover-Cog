#ifndef WORLDSTATE_H
#define WORLDSTATE_H

#include "gameconstants.h"
#include "mapdata.h"
#include "cropdefinition.h"

#include <QHash>
#include <QPoint>
#include <QSet>
#include <QVector>
#include <functional>

struct HarvestResult {
    CropType type = CropType::None;
    int quantity = 0;

    bool isValid() const { return type != CropType::None && quantity > 0; }
};

struct DayAdvanceResult {
    int cropsAdvanced = 0;
    int cropsStalled = 0;
    int soilRecovered = 0;

    bool changed() const { return cropsAdvanced > 0 || soilRecovered > 0; }
};

struct SavedCropState {
    QPoint tilePos;
    CropType type = CropType::None;
    int plantedDay = 0;
    int growthDays = 0;
};

struct WorldStateSnapshot {
    QVector<QVector<int>> cultivatedLayer;
    QVector<SavedCropState> crops;
    QSet<QPoint> wateredTiles;
    int lastCompletedDay = -1;
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
    bool plantSeed(const QPoint &tilePos, CropType cropType, int gameDay);
    bool waterTile(const QPoint &tilePos);
    bool isWatered(const QPoint &tilePos) const;
    HarvestResult previewHarvest(const QPoint &tilePos) const;
    HarvestResult harvest(const QPoint &tilePos);
    DayAdvanceResult advanceDay(int completedDay,
                                std::function<int()> randomPercent = {});
    int cropGidAt(const QPoint &tilePos) const;
    int treeGidAt(const QPoint &tilePos) const;
    WorldStateSnapshot snapshot() const;
    bool restore(const WorldStateSnapshot &snapshot);

private:
    struct TreeState {
        int health = 0;
    };

    struct CropState {
        CropType type = CropType::None;
        int plantedDay = 0;
        int growthDays = 0;
    };

    int m_mapWidth = 0;
    int m_mapHeight = 0;
    QVector<QVector<int>> m_cultivatableLayer;
    QVector<QVector<int>> m_cultivatedLayer;
    QVector<QVector<int>> m_cropLayer;
    QVector<QVector<int>> m_treeLayer;
    QVector<QVector<int>> m_collisionLayer;
    QHash<QPoint, TreeState> m_trees;
    QHash<QPoint, CropState> m_crops;
    QSet<QPoint> m_wateredTiles;
    int m_lastCompletedDay = -1;
};

#endif // WORLDSTATE_H
