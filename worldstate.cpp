#include "worldstate.h"

#include "gameconstants.h"

#include <QtGlobal>

namespace {

QVector<QVector<int>> normalizedLayer(const QVector<QVector<int>> &source,
                                      int width, int height) {
    QVector<QVector<int>> result(height, QVector<int>(width, 0));
    const int rows = qMin(height, source.size());
    for (int y = 0; y < rows; ++y) {
        const int columns = qMin(width, source.at(y).size());
        for (int x = 0; x < columns; ++x) {
            result[y][x] = source.at(y).at(x);
        }
    }
    return result;
}

} // namespace

void WorldState::initialize(const MapData &mapData) {
    m_mapWidth = qMax(0, mapData.mapWidth);
    m_mapHeight = qMax(0, mapData.mapHeight);
    m_cultivatableLayer = normalizedLayer(mapData.cultivatableLayer,
                                          m_mapWidth, m_mapHeight);
    m_cultivatedLayer = normalizedLayer(mapData.cultivatedLayer,
                                        m_mapWidth, m_mapHeight);
    m_cropLayer = normalizedLayer(mapData.cropLayer, m_mapWidth, m_mapHeight);
    m_collisionLayer = normalizedLayer(mapData.collisionLayer,
                                       m_mapWidth, m_mapHeight);

    m_trees.clear();
    m_plantingTimes.clear();
    m_plantedCropTypes.clear();

    m_treeLayer = normalizedLayer(mapData.treeLayer, m_mapWidth, m_mapHeight);
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            const int gid = m_treeLayer.at(y).at(x);
            if (gid == 0) {
                continue;
            }

            const auto property = mapData.tileProperties.constFind(gid);
            if (property != mapData.tileProperties.cend() && !property->interactable) {
                continue;
            }

            TreeState tree;
            tree.health = property == mapData.tileProperties.cend() || property->health <= 0
                              ? GameConstants::DefaultTreeHealth
                              : property->health;
            m_trees.insert(QPoint(x, y), tree);
        }
    }
}

bool WorldState::isValidTile(const QPoint &tilePos) const {
    return tilePos.x() >= 0 && tilePos.x() < m_mapWidth &&
           tilePos.y() >= 0 && tilePos.y() < m_mapHeight;
}

bool WorldState::hasTreeAt(const QPoint &tilePos) const {
    return m_trees.contains(tilePos);
}

bool WorldState::damageTree(const QPoint &tilePos, int damage) {
    auto tree = m_trees.find(tilePos);
    if (tree == m_trees.end() || damage <= 0) {
        return false;
    }
    tree->health -= damage;
    if (tree->health <= 0) {
        m_treeLayer[tilePos.y()][tilePos.x()] = 0;
        m_collisionLayer[tilePos.y()][tilePos.x()] = 0;
        m_trees.erase(tree);
    }
    return true;
}

bool WorldState::isTileCultivatable(const QPoint &tilePos) const {
    return isValidTile(tilePos) &&
           m_cultivatableLayer.at(tilePos.y()).at(tilePos.x()) ==
               GameConstants::CultivatableGrassGid;
}

bool WorldState::cultivate(const QPoint &tilePos) {
    if (!isTileCultivatable(tilePos)) {
        return false;
    }
    m_cultivatedLayer[tilePos.y()][tilePos.x()] = GameConstants::CultivatedTileGid;
    return true;
}

bool WorldState::isPlantable(const QPoint &tilePos) const {
    return isValidTile(tilePos) &&
           m_plantableGids.contains(m_cultivatedLayer.at(tilePos.y()).at(tilePos.x())) &&
           m_cropLayer.at(tilePos.y()).at(tilePos.x()) == 0;
}

bool WorldState::plantSeed(const QPoint &tilePos, CropType cropType,
                           qint64 gameMinute) {
    if (cropType == CropType::None || !isPlantable(tilePos)) {
        return false;
    }
    m_plantingTimes.insert(tilePos, gameMinute);
    m_plantedCropTypes.insert(tilePos, cropType);
    m_cropLayer[tilePos.y()][tilePos.x()] = GameConstants::CropFirstStageGid;
    return true;
}

CropType WorldState::harvest(const QPoint &tilePos) {
    if (!isValidTile(tilePos) ||
        m_cropLayer.at(tilePos.y()).at(tilePos.x()) != GameConstants::CropMatureStageGid) {
        return CropType::None;
    }
    m_cropLayer[tilePos.y()][tilePos.x()] = 0;
    m_plantingTimes.remove(tilePos);
    return m_plantedCropTypes.take(tilePos);
}

bool WorldState::advanceGrowth(qint64 gameMinute) {
    bool changed = false;
    for (auto it = m_plantingTimes.cbegin(); it != m_plantingTimes.cend(); ++it) {
        const QPoint tilePos = it.key();
        if (!isValidTile(tilePos)) {
            continue;
        }
        const qint64 elapsedMinutes = qMax<qint64>(0, gameMinute - it.value());
        const int stage = static_cast<int>(qMin<qint64>(
            elapsedMinutes / GameConstants::CropMinutesPerStage,
            GameConstants::CropGrowthStageCount - 1));
        const int newGid = GameConstants::CropFirstStageGid + stage;
        int &currentGid = m_cropLayer[tilePos.y()][tilePos.x()];
        if (currentGid != newGid) {
            currentGid = newGid;
            changed = true;
        }
    }
    return changed;
}

int WorldState::cropGidAt(const QPoint &tilePos) const {
    if (!isValidTile(tilePos)) {
        return 0;
    }
    return m_cropLayer.at(tilePos.y()).at(tilePos.x());
}

int WorldState::treeGidAt(const QPoint &tilePos) const {
    if (!isValidTile(tilePos)) {
        return 0;
    }
    return m_treeLayer.at(tilePos.y()).at(tilePos.x());
}
