#include "worldstate.h"

#include "gameconstants.h"

#include <QRandomGenerator>
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

bool isPlantableSoilGid(int gid) {
    switch (gid) {
    case 123:
    case 124:
    case 125:
    case 134:
    case GameConstants::CultivatedTileGid:
    case 136:
    case 145:
    case 146:
    case 147:
        return true;
    default:
        return false;
    }
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
    m_crops.clear();
    m_wateredTiles.clear();
    m_lastCompletedDay = -1;

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
           m_collisionLayer.at(tilePos.y()).at(tilePos.x()) == 0 &&
           m_cultivatableLayer.at(tilePos.y()).at(tilePos.x()) ==
               GameConstants::CultivatableGrassGid &&
           m_cultivatedLayer.at(tilePos.y()).at(tilePos.x()) == 0;
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
           isPlantableSoilGid(m_cultivatedLayer.at(tilePos.y()).at(tilePos.x())) &&
           m_cropLayer.at(tilePos.y()).at(tilePos.x()) == 0;
}

bool WorldState::plantSeed(const QPoint &tilePos, CropType cropType,
                           int gameDay) {
    const CropDefinition &definition = CropCatalog::definition(cropType);
    if (definition.stageGids.isEmpty() || !isPlantable(tilePos) ||
        gameDay <= m_lastCompletedDay || !CropCatalog::canPlantOnDay(cropType, gameDay)) {
        return false;
    }
    m_crops.insert(tilePos, CropState{cropType, gameDay, 0});
    m_cropLayer[tilePos.y()][tilePos.x()] = definition.stageGids.first();
    return true;
}

bool WorldState::waterTile(const QPoint &tilePos) {
    if (!isValidTile(tilePos) ||
        !isPlantableSoilGid(m_cultivatedLayer.at(tilePos.y()).at(tilePos.x()))) {
        return false;
    }
    m_wateredTiles.insert(tilePos);
    return true;
}

bool WorldState::isWatered(const QPoint &tilePos) const {
    return isValidTile(tilePos) && m_wateredTiles.contains(tilePos);
}

FertilizeResult WorldState::fertilize(const QPoint &tilePos) {
    if (!isValidTile(tilePos)) return {false, FertilizeFailure::InvalidTile};
    auto it = m_crops.find(tilePos);
    if (it == m_crops.end()) return {false, FertilizeFailure::NoCrop};
    if (it->fertilizerApplied) return {false, FertilizeFailure::AlreadyApplied};
    if (it->growthDays >= CropCatalog::definition(it->type).growthDays)
        return {false, FertilizeFailure::AlreadyMature};
    it->fertilizerApplied = true;
    it->fertilizerBonusPending = true;
    return {true, FertilizeFailure::None};
}

bool WorldState::isFertilized(const QPoint &tilePos) const {
    const auto it = m_crops.constFind(tilePos);
    return it != m_crops.cend() && it->fertilizerApplied;
}

bool WorldState::hasPendingFertilizerBonus(const QPoint &tilePos) const {
    const auto it = m_crops.constFind(tilePos);
    return it != m_crops.cend() && it->fertilizerBonusPending;
}

HarvestResult WorldState::previewHarvest(const QPoint &tilePos) const {
    if (!isValidTile(tilePos)) {
        return {};
    }
    const auto crop = m_crops.constFind(tilePos);
    if (crop == m_crops.cend()) return {};
    const CropType type = crop->type;
    const CropDefinition &definition = CropCatalog::definition(type);
    if (definition.type == CropType::None || definition.growthDays <= 0 ||
        crop->growthDays < definition.growthDays) {
        return {};
    }
    return {type, definition.harvestYield};
}

HarvestResult WorldState::harvest(const QPoint &tilePos) {
    const HarvestResult result = previewHarvest(tilePos);
    if (!result.isValid()) {
        return {};
    }
    m_cropLayer[tilePos.y()][tilePos.x()] = 0;
    m_crops.remove(tilePos);
    return result;
}

DayAdvanceResult WorldState::advanceDay(int completedDay,
                                        std::function<int()> randomPercent) {
    DayAdvanceResult result;
    if (completedDay <= m_lastCompletedDay) {
        return result;
    }
    m_lastCompletedDay = completedDay;

    for (auto it = m_crops.begin(); it != m_crops.end(); ++it) {
        const QPoint tilePos = it.key();
        CropState &crop = it.value();
        const CropDefinition &definition = CropCatalog::definition(crop.type);
        if (!isValidTile(tilePos) || definition.stageGids.isEmpty() ||
            definition.growthDays <= 0 || crop.growthDays >= definition.growthDays ||
            crop.plantedDay > completedDay) {
            continue;
        }
        if (!m_wateredTiles.contains(tilePos)) {
            ++result.cropsStalled;
            continue;
        }

        const int bonus = crop.fertilizerBonusPending ? 1 : 0;
        crop.growthDays = qMin(crop.growthDays + 1 + bonus, definition.growthDays);
        if (crop.fertilizerBonusPending) {
            crop.fertilizerBonusPending = false;
            ++result.fertilizerBonusesUsed;
        }
        ++result.cropsAdvanced;
        m_cropLayer[tilePos.y()][tilePos.x()] =
            CropCatalog::stageGid(crop.type, crop.growthDays);
    }

    if (!randomPercent) {
        randomPercent = []() {
            return QRandomGenerator::global()->bounded(100);
        };
    }
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            const QPoint tilePos(x, y);
            int &soilGid = m_cultivatedLayer[y][x];
            if (!isPlantableSoilGid(soilGid)) {
                continue;
            }
            if (!m_crops.contains(tilePos) &&
                qBound(0, randomPercent(), 99) <
                    GameConstants::EmptySoilRecoveryPercent) {
                soilGid = 0;
                ++result.soilRecovered;
            }
        }
    }
    m_wateredTiles.clear();
    return result;
}

DayAdvanceResult WorldState::advanceThroughDay(int completedDay) {
    if (qint64(completedDay) <= qint64(m_lastCompletedDay) + 1)
        return advanceDay(completedDay);

    // Match daily settlement, including the original random rolls for empty soil.
    // After the first day's watering, only empty soil can still change by itself.
    int emptySoil = 0;
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            if (isPlantableSoilGid(m_cultivatedLayer[y][x]) && !m_crops.contains({x, y}))
                ++emptySoil;
        }
    }
    DayAdvanceResult total;
    do {
        const auto day = advanceDay(m_lastCompletedDay + 1);
        total.cropsAdvanced += day.cropsAdvanced;
        total.cropsStalled += day.cropsStalled;
        total.soilRecovered += day.soilRecovered;
        total.fertilizerBonusesUsed += day.fertilizerBonusesUsed;
        emptySoil -= day.soilRecovered;
    } while (m_lastCompletedDay < completedDay && emptySoil > 0 &&
             GameConstants::EmptySoilRecoveryPercent > 0);

    if (m_lastCompletedDay < completedDay) {
        // With no water and no soil left to recover, every remaining date has
        // exactly the same farm state. Count the dry days without replaying frames.
        for (const auto &crop : m_crops) {
            if (crop.growthDays >= CropCatalog::definition(crop.type).growthDays)
                continue;
            const qint64 firstDryDay = qMax(qint64(m_lastCompletedDay) + 1, qint64(crop.plantedDay));
            total.cropsStalled += qMax(qint64(0), qint64(completedDay) - firstDryDay + 1);
        }
        m_lastCompletedDay = completedDay;
    }
    return total;
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

WorldStateSnapshot WorldState::snapshot() const {
    WorldStateSnapshot result;
    result.cultivatedLayer = m_cultivatedLayer;
    result.wateredTiles = m_wateredTiles;
    result.lastCompletedDay = m_lastCompletedDay;
    result.crops.reserve(m_crops.size());
    for (auto it = m_crops.cbegin(); it != m_crops.cend(); ++it) {
        result.crops.append(
            {it.key(), it->type, it->plantedDay, it->growthDays,
             it->fertilizerApplied, it->fertilizerBonusPending});
    }
    return result;
}

bool WorldState::restore(const WorldStateSnapshot &snapshot) {
    if (snapshot.cultivatedLayer.size() != m_mapHeight ||
        snapshot.lastCompletedDay < -1) {
        return false;
    }
    for (const QVector<int> &row : snapshot.cultivatedLayer) {
        if (row.size() != m_mapWidth) {
            return false;
        }
        for (int gid : row) {
            if (gid != 0 && !isPlantableSoilGid(gid)) {
                return false;
            }
        }
    }

    QHash<QPoint, CropState> restoredCrops;
    QVector<QVector<int>> restoredCropLayer(
        m_mapHeight, QVector<int>(m_mapWidth, 0));
    for (const SavedCropState &savedCrop : snapshot.crops) {
        const CropDefinition &definition =
            CropCatalog::definition(savedCrop.type);
        if (!isValidTile(savedCrop.tilePos) || definition.stageGids.isEmpty() ||
            definition.growthDays <= 0 ||
            savedCrop.plantedDay < 0 || savedCrop.growthDays < 0 ||
            savedCrop.growthDays > definition.growthDays ||
            (savedCrop.fertilizerBonusPending &&
             (!savedCrop.fertilizerApplied || savedCrop.growthDays >= definition.growthDays)) ||
            restoredCrops.contains(savedCrop.tilePos) ||
            !isPlantableSoilGid(
                snapshot.cultivatedLayer.at(savedCrop.tilePos.y())
                    .at(savedCrop.tilePos.x()))) {
            return false;
        }
        restoredCropLayer[savedCrop.tilePos.y()][savedCrop.tilePos.x()] =
            CropCatalog::stageGid(savedCrop.type, savedCrop.growthDays);
        restoredCrops.insert(savedCrop.tilePos,
                             CropState{savedCrop.type, savedCrop.plantedDay,
                                       savedCrop.growthDays, savedCrop.fertilizerApplied,
                                       savedCrop.fertilizerBonusPending});
    }

    for (const QPoint &tilePos : snapshot.wateredTiles) {
        if (!isValidTile(tilePos) ||
            !isPlantableSoilGid(
                snapshot.cultivatedLayer.at(tilePos.y()).at(tilePos.x()))) {
            return false;
        }
    }

    m_cultivatedLayer = snapshot.cultivatedLayer;
    m_cropLayer = restoredCropLayer;
    m_crops = restoredCrops;
    m_wateredTiles = snapshot.wateredTiles;
    m_lastCompletedDay = snapshot.lastCompletedDay;
    return true;
}
