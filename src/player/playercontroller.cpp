#include "playercontroller.h"

#include "map/mapwidget.h"

namespace {
ToolProperty propertyForTool(ToolType type, bool axeUpgraded) {
    ToolProperty tool{type, toolName(type), 0, {}};
    switch (type) {
    case ToolType::EmptyHand: break;
    case ToolType::Axe:
        tool.damage = axeUpgraded ? GameConstants::UpgradedAxeDamage : GameConstants::BaseAxeDamage;
        break;
    case ToolType::Hoe: tool.damage = 15; break;
    case ToolType::WateringCan: tool.damage = 5; break;
    default: return tool;
    }
    tool.animFile = QStringLiteral(":/characte/Premium Charakter Spritesheet.png");
    return tool;
}
}

PlayerController::PlayerController(MapWidget *mapWidget, QObject *parent)
    : QObject(parent), m_mapWidget(mapWidget), m_currentTool(propertyForTool(ToolType::EmptyHand, false)) {
    if (m_mapWidget)
        m_mapWidget->setPlayerController(this);
}

void PlayerController::setCurrentTool(int toolIndex) {
    const ToolType tool = static_cast<ToolType>(toolIndex);
    if (tool != ToolType::EmptyHand && tool != ToolType::Axe && toolSlot(tool) < 0)
        return;
    m_currentTool = propertyForTool(tool, m_axeUpgraded);
    emit toolChanged(m_currentTool.type);
}

void PlayerController::setAxeUpgraded(bool upgraded) {
    if (m_axeUpgraded == upgraded) return;
    m_axeUpgraded = upgraded;
    if (m_currentTool.type == ToolType::Axe)
        m_currentTool = propertyForTool(ToolType::Axe, upgraded);
}

void PlayerController::useToolAt(const QPoint &mapPoint) {
    if (!m_mapWidget || m_mapWidget->isInputBlocked()) {
        return;
    }
    const QPoint tilePos = m_mapWidget->mapPointToTile(mapPoint);
    if (!m_mapWidget->isValidTile(tilePos)) {
        return;
    }
    if (!canReachTile(tilePos)) {
        emit m_mapWidget->farmingFeedback(QStringLiteral("距离太远，请靠近到周围一格内操作"));
        return;
    }

    const auto crop = seedCropForTool(m_currentTool.type);
    if (crop != CropType::None) {
        m_mapWidget->plantSeed(tilePos, crop);
        return;
    }
    switch (m_currentTool.type) {
    case ToolType::Axe:
        m_mapWidget->chopTree(tilePos);
        break;
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

bool PlayerController::canReachTile(const QPoint &tile) const {
    if (!m_tilePosition || !m_mapWidget || !m_mapWidget->isValidTile(tile)) return false;
    const QPoint player = m_tilePosition();
    return m_mapWidget->isValidTile(player) &&
           qAbs(qint64(tile.x()) - player.x()) <= 1 &&
           qAbs(qint64(tile.y()) - player.y()) <= 1;
}
