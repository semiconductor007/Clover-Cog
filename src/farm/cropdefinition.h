#ifndef CROPDEFINITION_H
#define CROPDEFINITION_H

#include "seasoncalendar.h"
#include <QRect>
#include <QString>
#include <QVector>

enum class CropType {
    None = 0,
    Wheat = 1,
    Potato = 2,
    Carrot = 3,
    Beet = 4
};

struct CropIcon {
    QString resourcePath;
    QRect sourceRect;
};

struct CropDefinition {
    CropType type = CropType::None;
    QString id;
    QString displayName;
    int growthDays = 0;
    int harvestYield = 0;
    int seedPrice = 0;
    int sellPrice = 0;
    QVector<int> stageGids;
    QString localizedName;
    quint8 plantingSeasons = 0;
    CropIcon seedIcon;
    CropIcon harvestIcon;
};

class CropCatalog {
public:
    static const CropDefinition &definition(CropType type);
    static QVector<CropType> cropTypes();
    static CropType fromId(const QString &id);
    static bool canPlant(CropType type, Season season);
    static bool canPlantOnDay(CropType type, qint64 zeroBasedGameDay);
    static int stageGid(CropType type, int effectiveGrowthDays);
};

#endif // CROPDEFINITION_H
