#include "playercontroller.h"

#include "mapwidget.h"



PlayerController::PlayerController(MapWidget *mapWidget, QObject *parent)
    : QObject(parent), m_mapWidget(mapWidget) {
    for (int i = 0; i < ToolCount; ++i) {
        const auto type = static_cast<ToolType>(i);
        const bool equipment = i <= static_cast<int>(ToolType::WateringCan);
        const int damage = type == ToolType::Axe ? 30 :
                           type == ToolType::Hoe ? 15 :
                           type == ToolType::WateringCan ? 5 : 0;
        m_tools.append({type, toolName(type), damage,
                        equipment ? QStringLiteral(":/characte/Premium Charakter Spritesheet.png")
                                  : QString()});
    }
    m_currentTool = m_tools.first();
}

void PlayerController::setCurrentTool(int toolIndex) {
    if (toolIndex < 0 || toolIndex >= m_tools.size()) {
        return;
    }
    m_currentTool = m_tools.at(toolIndex);
    emit toolChanged(m_currentTool.type);
}

void PlayerController::useToolAt(const QPoint &mapPoint) {
    if (!m_mapWidget) {
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

void PlayerController::useAxeAt(const QPoint &playerCenter, PlayerDirection direction) {
    if (!m_mapWidget || m_currentTool.type != ToolType::Axe ||
        m_mapWidget->tileWidth() <= 0 || m_mapWidget->tileHeight() <= 0) {
        return;
    }

    QPoint target(playerCenter.x() / m_mapWidget->tileWidth(),
                  playerCenter.y() / m_mapWidget->tileHeight());
    switch (direction) {
    case PlayerDirection::Up: target.ry()--; break;
    case PlayerDirection::Down: target.ry()++; break;
    case PlayerDirection::Left: target.rx()--; break;
    case PlayerDirection::Right: target.rx()++; break;
    }

    if (m_mapWidget->hasTreeAt(target)) {
        m_mapWidget->modifyTreeHealth(target, m_currentTool.damage);
    }
}
