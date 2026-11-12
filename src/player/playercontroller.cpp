#include "playercontroller.h"

#include "map/mapwidget.h"

namespace {
ToolProperty propertyForTool(ToolType type) {
    ToolProperty tool{type, toolName(type), 0, {}};
    switch (type) {
    case ToolType::EmptyHand: break;
    case ToolType::Hoe: tool.damage = 15; break;
    case ToolType::WateringCan: tool.damage = 5; break;
    default: return tool;
    }
    tool.animFile = QStringLiteral(":/characte/Premium Charakter Spritesheet.png");
    return tool;
}
}

PlayerController::PlayerController(MapWidget *mapWidget, QObject *parent)
    : QObject(parent), m_mapWidget(mapWidget), m_currentTool(propertyForTool(ToolType::EmptyHand)) {
    if (m_mapWidget)
        m_mapWidget->setPlayerController(this);
}

void PlayerController::setCurrentTool(int toolIndex) {
    const ToolType tool = static_cast<ToolType>(toolIndex);
    if (toolSlot(tool) < 0)
        return;
    m_currentTool = propertyForTool(tool);
    emit toolChanged(m_currentTool.type);
}

void PlayerController::useToolAt(const QPoint &mapPoint) {
    if (!m_mapWidget || m_mapWidget->isInputBlocked()) {
        return;
    }
    const QPoint tilePos = m_mapWidget->mapPointToTile(mapPoint);
    if (!m_mapWidget->isValidTile(tilePos)) {
        return;
    }

    const auto crop = seedCropForTool(m_currentTool.type);
    if (crop != CropType::None) {
        m_mapWidget->plantSeed(tilePos, crop);
        return;
    }
    switch (m_currentTool.type) {
    case ToolType::Hoe:
        m_mapWidget->cultivateTile(tilePos);
        break;
    case ToolType::Fertilizer:
        m_mapWidget->fertilizeTile(tilePos);
        break;
    case ToolType::WateringCan:
        m_mapWidget->waterTile(tilePos);
        break;
    default:
        break;
    }
}
