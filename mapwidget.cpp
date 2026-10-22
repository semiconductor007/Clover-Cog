#include "mapwidget.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent), m_growthTimer(new QTimer(this)) {
    setAttribute(Qt::WA_InputMethodEnabled, false);
    connect(m_growthTimer, &QTimer::timeout, this, &MapWidget::updateGrowthStages);
    m_growthTimer->start(1000);
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
    m_tilesets = std::move(data.tilesets);
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

void MapWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    m_renderer.paint(painter, rect(), QPoint(m_offsetX, m_offsetY),
                     m_mapWidth, m_mapHeight, m_tileWidth, m_tileHeight,
                     m_tilesets, m_renderLayers, m_worldState);
}

void MapWidget::setViewportCenter(int x, int y) {
    const int mapPixelWidth = m_mapWidth * m_tileWidth;
    const int mapPixelHeight = m_mapHeight * m_tileHeight;
    const int maxOffsetX = qMax(0, mapPixelWidth - width());
    const int maxOffsetY = qMax(0, mapPixelHeight - height());

    m_offsetX = qBound(0, x - width() / 2, maxOffsetX);
    m_offsetY = qBound(0, y - height() / 2, maxOffsetY);
    update();
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        const QPoint tilePos = mapPointToTile(event->position().toPoint());
        if (!isValidTile(tilePos)) {
            event->ignore();
            return;
        }

        const CropType cropType = m_worldState.harvest(tilePos);
        if (cropType != CropType::None) {
            emit cropHarvested(cropType);
            update();
        }
    }

    emit mapClicked(event->pos());
    QWidget::mousePressEvent(event);
}

QPoint MapWidget::mapPointToTile(const QPoint &mapPoint) const {
    if (m_tileWidth <= 0 || m_tileHeight <= 0) {
        return QPoint(-1, -1);
    }
    return QPoint((mapPoint.x() + m_offsetX) / m_tileWidth,
                  (mapPoint.y() + m_offsetY) / m_tileHeight);
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

bool MapWidget::cultivateTile(const QPoint &tilePos) {
    if (!m_worldState.cultivate(tilePos)) {
        return false;
    }
    update();
    return true;
}

bool MapWidget::isTileCultivatable(const QPoint &tilePos) const {
    return m_worldState.isTileCultivatable(tilePos);
}

bool MapWidget::isPlantable(const QPoint &tilePos) const {
    return m_worldState.isPlantable(tilePos);
}

bool MapWidget::plantSeed(const QPoint &tilePos, CropType cropType) {
    const qint64 gameMinute = m_getGameMinutes ? m_getGameMinutes() : 0;
    if (!m_worldState.plantSeed(tilePos, cropType, gameMinute)) {
        return false;
    }
    update();
    return true;
}

void MapWidget::updateGrowthStages() {
    const qint64 now = m_getGameMinutes ? m_getGameMinutes() : 0;
    if (m_worldState.advanceGrowth(now)) {
        update();
    }
}
