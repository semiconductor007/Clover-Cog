#ifndef CROPDEFINITION_H
#define CROPDEFINITION_H

#include <QString>
#include <QVector>

enum class CropType {
    None,
    Wheat,
    Potato
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
};

class CropCatalog {
public:
    static const CropDefinition &definition(CropType type);
    static QVector<CropType> cropTypes();
};

#endif // CROPDEFINITION_H
