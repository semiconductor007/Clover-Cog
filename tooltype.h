// tooltype.h
#ifndef TOOLTYPE_H
#define TOOLTYPE_H

#include "cropdefinition.h"
#include <QString>
#include <QPixmap>
#include <QVector>

enum class ToolType {
    EmptyHand,  // 空手
    Axe,        // 斧子
    Hoe,        // 锄头
    WateringCan,// 水壶
    Wheat,
    Potato,
    Carrot,
    Beet,
    Fertilizer
};

inline constexpr CropType seedCropForTool(ToolType tool) {
    switch (tool) {
    case ToolType::Wheat: return CropType::Wheat;
    case ToolType::Potato: return CropType::Potato;
    case ToolType::Carrot: return CropType::Carrot;
    case ToolType::Beet: return CropType::Beet;
    default: return CropType::None;
    }
}

inline constexpr ToolType toolForSeed(CropType crop) {
    switch (crop) {
    case CropType::Wheat: return ToolType::Wheat;
    case CropType::Potato: return ToolType::Potato;
    case CropType::Carrot: return ToolType::Carrot;
    case CropType::Beet: return ToolType::Beet;
    default: return ToolType::EmptyHand;
    }
}

struct ToolAnimation {
    QVector<QPixmap> leftFrames;  // 向左动画帧
    QVector<QPixmap> rightFrames; // 向右动画帧
    QVector<QPixmap> upFrames;    // 向上动画帧
    QVector<QPixmap> downFrames;  // 向下动画帧
};

struct ToolProperty {
    ToolType type;
    QString name;
    int damage;      // 工具伤害值
    QString animFile;// 对应动画文件路径
};

#endif // TOOLTYPE_H
