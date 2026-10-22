#include "playercontroller.h"

#include "mapwidget.h"

#include <QDebug>

PlayerController::PlayerController(MapWidget *mapWidget, QObject *parent)
    : QObject(parent), m_mapWidget(mapWidget) {
    m_tools = {
        {ToolType::EmptyHand,   "空手",     0,  ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::Axe,         "斧子",     30, ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::Hoe,         "锄头",     15, ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::WateringCan, "水壶",     5,  ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::Wheat,       "小麦种子", 0,  ":/anim/planting.png"},
        {ToolType::Potato,      "土豆种子", 0,  ":/anim/planting.png"}
    };
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

    switch (m_currentTool.type) {
    case ToolType::Hoe:
        if (m_mapWidget->isTileCultivatable(tilePos)) {
            m_mapWidget->cultivateTile(tilePos);
        }
        break;
    case ToolType::Wheat:
        if (m_mapWidget->isPlantable(tilePos)) {
            m_mapWidget->plantSeed(tilePos, CropType::Wheat);
        }
        break;
    case ToolType::Potato:
        if (m_mapWidget->isPlantable(tilePos)) {
            m_mapWidget->plantSeed(tilePos, CropType::Potato);
        }
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
