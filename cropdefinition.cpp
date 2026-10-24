#include "cropdefinition.h"

#include "gameconstants.h"

namespace {

const CropDefinition noneDefinition;

const CropDefinition wheatDefinition{
    CropType::Wheat,
    QStringLiteral("wheat"),
    QStringLiteral("Wheat"),
    3,
    1,
    2,
    4,
    {GameConstants::CropFirstStageGid, GameConstants::CropFirstStageGid + 1,
     GameConstants::CropFirstStageGid + 2, GameConstants::CropMatureStageGid}
};

const CropDefinition potatoDefinition{
    CropType::Potato,
    QStringLiteral("potato"),
    QStringLiteral("Potato"),
    5,
    3,
    5,
    3,
    {GameConstants::CropFirstStageGid, GameConstants::CropFirstStageGid + 1,
     GameConstants::CropFirstStageGid + 2, GameConstants::CropMatureStageGid}
};

} // namespace

const CropDefinition &CropCatalog::definition(CropType type) {
    switch (type) {
    case CropType::Wheat:
        return wheatDefinition;
    case CropType::Potato:
        return potatoDefinition;
    case CropType::None:
        return noneDefinition;
    }
    return noneDefinition;
}

QVector<CropType> CropCatalog::cropTypes() {
    return {CropType::Wheat, CropType::Potato};
}
