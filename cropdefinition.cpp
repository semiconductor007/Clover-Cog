#include "cropdefinition.h"
#include "gameconstants.h"

namespace {

const CropDefinition noneDefinition;
constexpr quint8 spring = SeasonCalendar::seasonBit(Season::Spring);
constexpr quint8 summer = SeasonCalendar::seasonBit(Season::Summer);
constexpr quint8 autumn = SeasonCalendar::seasonBit(Season::Autumn);
constexpr quint8 winter = SeasonCalendar::seasonBit(Season::Winter);
const QString sproutItems = QStringLiteral(":/tilesets/sprout/Crop_Items.png");
const QString springCrops = QStringLiteral(":/tilesets/sprout/Spring_Crops.png");

const CropDefinition wheatDefinition{
    CropType::Wheat, QStringLiteral("wheat"), QStringLiteral("Wheat"),
    3, 1, 2, 4,
    {GameConstants::CropFirstStageGid, GameConstants::CropFirstStageGid + 1,
     GameConstants::CropFirstStageGid + 2, GameConstants::CropMatureStageGid},
    QStringLiteral("小麦"), SeasonCalendar::AllSeasons,
    {sproutItems, QRect(0, 128, 16, 16)},
    {sproutItems, QRect(16, 128, 16, 16)}
};

// The supplied Farm RPG sheet has real potato stages and item icons.
// Its tileset is appended at firstgid 789; legacy map GIDs remain stable.
const CropDefinition potatoDefinition{
    CropType::Potato, QStringLiteral("potato"), QStringLiteral("Potato"),
    5, 3, 5, 3, {859, 861, 863, 865},
    QStringLiteral("土豆"), spring | summer,
    {springCrops, QRect(112, 80, 16, 16)},
    {springCrops, QRect(128, 80, 16, 16)}
};

const CropDefinition carrotDefinition{
    CropType::Carrot, QStringLiteral("carrot"), QStringLiteral("Carrot"),
    4, 2, 3, 3, {210, 211, 212, 213},
    QStringLiteral("胡萝卜"), spring | autumn,
    {sproutItems, QRect(0, 32, 16, 16)},
    {sproutItems, QRect(16, 32, 16, 16)}
};

const CropDefinition beetDefinition{
    CropType::Beet, QStringLiteral("beet"), QStringLiteral("Beet"),
    4, 2, 4, 4, {220, 221, 222, 223},
    QStringLiteral("甜菜"), autumn | winter,
    {sproutItems, QRect(0, 64, 16, 16)},
    {sproutItems, QRect(16, 64, 16, 16)}
};

} // namespace

const CropDefinition &CropCatalog::definition(CropType type) {
    switch (type) {
    case CropType::Wheat: return wheatDefinition;
    case CropType::Potato: return potatoDefinition;
    case CropType::Carrot: return carrotDefinition;
    case CropType::Beet: return beetDefinition;
    case CropType::None: return noneDefinition;
    }
    return noneDefinition;
}

QVector<CropType> CropCatalog::cropTypes() {
    return {CropType::Wheat, CropType::Potato, CropType::Carrot, CropType::Beet};
}

CropType CropCatalog::fromId(const QString &id) {
    for (CropType type : cropTypes()) {
        if (definition(type).id == id) return type;
    }
    return CropType::None;
}

bool CropCatalog::canPlant(CropType type, Season season) {
    return definition(type).type != CropType::None &&
           (definition(type).plantingSeasons & SeasonCalendar::seasonBit(season)) != 0;
}

bool CropCatalog::canPlantOnDay(CropType type, qint64 zeroBasedGameDay) {
    return zeroBasedGameDay >= 0 &&
           canPlant(type, SeasonCalendar::dateForDay(zeroBasedGameDay).season);
}

int CropCatalog::stageGid(CropType type, int effectiveGrowthDays) {
    const CropDefinition &crop = definition(type);
    if (crop.growthDays <= 0 || crop.stageGids.isEmpty() || effectiveGrowthDays < 0)
        return 0;
    const int day = qMin(effectiveGrowthDays, crop.growthDays);
    const int stage = day * (crop.stageGids.size() - 1) / crop.growthDays;
    return crop.stageGids.at(stage);
}
