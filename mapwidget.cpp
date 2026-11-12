#include "mapwidget.h"
#include "playercontroller.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_InputMethodEnabled, false);
}

void MapWidget::setPlayerController(PlayerController *controller) {
    m_playerController = controller;
}

bool MapWidget::loadMap(const QString &tmjPath) {
    MapData data;
    QString errorMessage;
    if (!MapLoader::load(tmjPath, &data, &errorMessage)) {
        qWarning() << errorMessage;
        emit mapLoadFailed(errorMessage);
        return false;
    }

    m_tileWidth = data.tileWidth;
    m_tileHeight = data.tileHeight;
    m_mapWidth = data.mapWidth;
    m_mapHeight = data.mapHeight;
    m_worldState.initialize(data);
    m_originalTilesets = std::move(data.tilesets);
    m_displayTilesets = m_originalTilesets;
    m_paintScenery = {};
    m_renderLayers = std::move(data.renderLayers);
    m_offsetX = 0;
    m_offsetY = 0;

    updateGeometry();
    update();
    return true;
}

bool MapWidget::hasTreeAt(const QPoint &tilePos) const {
    return m_worldState.hasTreeAt(tilePos);
}

bool MapWidget::modifyTreeHealth(const QPoint &tilePos, int damage) {
    if (!m_worldState.damageTree(tilePos, damage)) {
        return false;
    }
    update();
    return true;
}

void MapWidget::setSeasonAppearance(QVector<Tileset> displayTilesets,
                                    std::function<void(QPainter &, const QPoint &, const WorldState &)> paintScenery) {
    m_displayTilesets = displayTilesets.isEmpty() ? m_originalTilesets : std::move(displayTilesets);
    m_paintScenery = std::move(paintScenery);
    update();
}

void MapWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    m_renderer.paint(painter, rect(), QPoint(m_offsetX, m_offsetY),
                     m_mapWidth, m_mapHeight, m_tileWidth, m_tileHeight,
                     m_displayTilesets, m_renderLayers, m_worldState);
    if (m_paintScenery) {
        painter.save();
        m_paintScenery(painter, QPoint(m_offsetX, m_offsetY), m_worldState);
        painter.restore();
    }

    const auto drawInteractionArea =
        [this, &painter](const QRect &tileArea, const QColor &tint,
                         const QString &text) {
        if (tileArea.isEmpty() || m_tileWidth <= 0 || m_tileHeight <= 0) {
            return;
        }
        const QRect screenRect(tileArea.x() * m_tileWidth - m_offsetX,
                               tileArea.y() * m_tileHeight - m_offsetY,
                               tileArea.width() * m_tileWidth,
                               tileArea.height() * m_tileHeight);
        painter.save();
        painter.fillRect(screenRect, tint);
        painter.setPen(QColor(92, 55, 32));
        painter.drawRect(screenRect.adjusted(0, 0, -1, -1));

        // The map supplies the real furniture; markers only identify where to stand.
        QFont labelFont = painter.font();
        labelFont.setPixelSize(8);
        painter.setFont(labelFont);
        const QRect labelRect(screenRect.center().x() - 36,
                              screenRect.top() - 12, 72, 12);
        painter.fillRect(labelRect, QColor(76, 48, 30, 220));
        painter.setPen(Qt::white);
        painter.drawText(labelRect, Qt::AlignCenter, text);
        painter.restore();
    };
    drawInteractionArea(m_marketArea, QColor(255, 222, 126, 45),
                        QStringLiteral("收购 [E]"));
    drawInteractionArea(m_shopArea, QColor(114, 190, 255, 45),
                        QStringLiteral("商店 [E]"));
    drawInteractionArea(m_storageArea, QColor(135, 211, 173, 45),
                        QStringLiteral("储物 [E]"));
    drawInteractionArea(m_bedArea, QColor(180, 130, 220, 45),
                        QStringLiteral("床铺 [E]"));
}

void MapWidget::setViewportCenter(int x, int y) {
    const int mapPixelWidth = m_mapWidth * m_tileWidth;
    const int mapPixelHeight = m_mapHeight * m_tileHeight;
    const int maxOffsetX = qMax(0, mapPixelWidth - width());
    const int maxOffsetY = qMax(0, mapPixelHeight - height());

    m_offsetX = qBound(0, qMax(0, x) - width() / 2, maxOffsetX);
    m_offsetY = qBound(0, qMax(0, y) - height() / 2, maxOffsetY);
    update();
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    event->accept();
    if (m_inputBlocked || event->button() != Qt::LeftButton)
        return;

    const QPointF local = event->position();
    const QPoint point(qFloor(local.x()), qFloor(local.y()));
    const QPoint tilePos = mapPointToTile(point);
    if (!isValidTile(tilePos))
        return;

    // Fertilizer always goes through the crop and inventory checks, even on ripe crops.
    if (m_playerController && m_playerController->currentTool().type == ToolType::Fertilizer) {
        emit mapClicked(point);
        return;
    }

    // For other tools a mature crop owns the click, even when the backpack is full.
    const HarvestResult preview = m_worldState.previewHarvest(tilePos);
    if (preview.isValid()) {
        if (m_harvestHandler && !m_harvestHandler(preview.type, preview.quantity)) {
            emit harvestBlocked(preview.type, preview.quantity);
            return;
        }
        const HarvestResult harvested = m_worldState.harvest(tilePos);
        emit cropHarvested(harvested.type, harvested.quantity);
        emit worldChanged();
        update();
        return;
    }
    emit mapClicked(point);
}

QPoint MapWidget::mapPointToTile(const QPoint &mapPoint) const {
    if (m_tileWidth <= 0 || m_tileHeight <= 0 || !rect().contains(mapPoint))
        return QPoint(-1, -1);

    const QPoint world = mapPoint + QPoint(m_offsetX, m_offsetY);
    const QPoint tile(world.x() / m_tileWidth, world.y() / m_tileHeight);
    return isValidTile(tile) ? tile : QPoint(-1, -1);
}

bool MapWidget::isValidTile(const QPoint &tilePos) const {
    return m_worldState.isValidTile(tilePos);
}

int MapWidget::tileAt(int x, int y) const {
    if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight ||
        m_renderLayers.isEmpty()) {
        return -1;
    }

    const QVector<QVector<int>> &layer = m_renderLayers.first().tiles;
    if (y >= layer.size() || x >= layer.at(y).size()) {
        return -1;
    }
    return layer.at(y).at(x);
}

bool MapWidget::isInteractionTile(const QPoint &tilePos) const {
    return m_marketArea.contains(tilePos) || m_shopArea.contains(tilePos) ||
           m_bedArea.contains(tilePos) || m_storageArea.contains(tilePos);
}

bool MapWidget::cultivateTile(const QPoint &tilePos) {
    if (isInteractionTile(tilePos)) {
        emit farmingFeedback(QStringLiteral("互动区域不能耕种，请选草地"));
        return false;
    }
    if (m_worldState.cultivate(tilePos)) {
        update();
        emit farmingFeedback(QStringLiteral("已翻土，可以播种"));
        emit worldChanged();
        return true;
    }

    if (m_worldState.isPlantable(tilePos))
        emit farmingFeedback(QStringLiteral("这里可以直接播种"));
    else if (m_worldState.isPathTile(tilePos))
        emit farmingFeedback(QStringLiteral("道路不能耕种，请选草地"));
    else if (m_worldState.cropGidAt(tilePos) != 0)
        emit farmingFeedback(QStringLiteral("这里已经种有作物"));
    else
        emit farmingFeedback(QStringLiteral("这里不能耕种，请选草地"));
    return false;
}

bool MapWidget::isTileCultivatable(const QPoint &tilePos) const {
    return !isInteractionTile(tilePos) && m_worldState.isTileCultivatable(tilePos);
}

bool MapWidget::isPlantable(const QPoint &tilePos) const {
    return !isInteractionTile(tilePos) && m_worldState.isPlantable(tilePos);
}

bool MapWidget::plantSeed(const QPoint &tilePos, CropType cropType) {
    const qint64 gameMinute = m_getGameMinutes ? m_getGameMinutes() : 0;
    const int gameDay = static_cast<int>(gameMinute / (24 * 60));
    if (!isPlantable(tilePos)) {
        return false;
    }
    if (!CropCatalog::canPlantOnDay(cropType, gameDay)) {
        emit plantingOutOfSeason(cropType);
        return false;
    }
    const auto plantAction = [this, tilePos, cropType, gameDay]() {
        return m_worldState.plantSeed(tilePos, cropType, gameDay);
    };
    if (m_plantHandler && !m_plantHandler(cropType, plantAction)) {
        emit plantingBlocked(cropType);
        return false;
    }
    if (!m_plantHandler && !plantAction()) {
        return false;
    }
    update();
    emit worldChanged();
    return true;
}

bool MapWidget::fertilizeTile(const QPoint &tilePos) {
    WorldState candidate = m_worldState;
    FertilizeResult result;
    bool attempted = false;
    // The inventory owner checks stock, calls the effect synchronously, then
    // consumes one item only if it succeeded. Notify observers after both writes.
    const auto apply = [&]() {
        if (attempted)
            return false;
        attempted = true;
        result = candidate.fertilize(tilePos);
        return result.success;
    };
    if (m_fertilizeHandler && m_fertilizeHandler(apply) && result.success) {
        m_worldState = std::move(candidate);
        update();
        emit worldChanged();
        emit farmingFeedback(QStringLiteral("已施肥，下次浇水结算时加快生长"));
        return true;
    }
    QString reason;
    if (!attempted) {
        reason = QStringLiteral("肥料不足，请到商店购买");
    } else {
        switch (result.failure) {
        case FertilizeFailure::InvalidTile: reason = QStringLiteral("此处不能施肥"); break;
        case FertilizeFailure::NoCrop: reason = QStringLiteral("请对未成熟的作物施肥"); break;
        case FertilizeFailure::AlreadyApplied: reason = QStringLiteral("这株作物已经施过肥"); break;
        case FertilizeFailure::AlreadyMature: reason = QStringLiteral("作物已成熟，可以收获"); break;
        case FertilizeFailure::None: reason = QStringLiteral("施肥失败"); break;
        }
    }
    emit farmingFeedback(reason);
    return false;
}

bool MapWidget::waterTile(const QPoint &tilePos) {
    if (isInteractionTile(tilePos) || m_worldState.isWatered(tilePos) ||
        !m_worldState.waterTile(tilePos)) {
        return false;
    }
    update();
    emit worldChanged();
    emit tileWatered();
    return true;
}

DayAdvanceResult MapWidget::advanceDay(int completedDay,
                                       std::function<int()> randomPercent) {
    const DayAdvanceResult result =
        m_worldState.advanceDay(completedDay, std::move(randomPercent));
    update();
    return result;
}

DayAdvanceResult MapWidget::advanceThroughDay(int completedDay) {
    const auto result = m_worldState.advanceThroughDay(completedDay);
    update();
    return result;
}
