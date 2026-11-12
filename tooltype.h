// tooltype.h
#ifndef TOOLTYPE_H
#define TOOLTYPE_H

#include "cropdefinition.h"
#include <QString>
#include <QPixmap>
#include <QVector>
#include <array>

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

// The IDs are stable across controls and inventory. Only these tools have player slots.
inline constexpr int ToolCount = static_cast<int>(ToolType::Fertilizer) + 1;
inline constexpr std::array PlayableTools{
    ToolType::EmptyHand, ToolType::Hoe, ToolType::WateringCan, ToolType::Wheat,
    ToolType::Potato, ToolType::Carrot, ToolType::Beet, ToolType::Fertilizer
};
inline constexpr int ToolbarSlotCount = static_cast<int>(PlayableTools.size());

inline constexpr int toolSlot(ToolType tool) {
    for (int i = 0; i < ToolbarSlotCount; ++i) {
        if (PlayableTools[i] == tool)
            return i;
    }
    return -1;
}

inline QString toolName(ToolType tool) {
    const CropType crop = seedCropForTool(tool);
    if (crop != CropType::None)
        return CropCatalog::definition(crop).localizedName + QStringLiteral("种子");
    switch (tool) {
    case ToolType::EmptyHand: return QStringLiteral("空手");
    case ToolType::Axe: return QStringLiteral("斧子");
    case ToolType::Hoe: return QStringLiteral("锄头");
    case ToolType::WateringCan: return QStringLiteral("水壶");
    case ToolType::Fertilizer: return QStringLiteral("基础肥料");
    default: return {};
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
