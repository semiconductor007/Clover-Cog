#include "mapwidget.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent),
      m_marketIcon(QStringLiteral(":/tilesets/sprout/Chest.png")),
      m_bedIcon(QStringLiteral(":/tilesets/sprout/Basic_Furniture.png")) {
    setAttribute(Qt::WA_InputMethodEnabled, false);
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

    const auto drawInteractionArea =
        [this, &painter](const QRect &tileArea, const QColor &tint,
                         const QString &text, const QPixmap &icon,
                         const QRect &iconSource) {
        if (tileArea.isEmpty() || m_tileWidth <= 0 || m_tileHeight <= 0) {
            return;
        }
        const QRect screenRect(tileArea.x() * m_tileWidth - m_offsetX,
                               tileArea.y() * m_tileHeight - m_offsetY,
                               tileArea.width() * m_tileWidth,
                               tileArea.height() * m_tileHeight);
        painter.fillRect(screenRect, tint);
        painter.setPen(QColor(92, 55, 32));
        painter.drawRect(screenRect.adjusted(0, 0, -1, -1));
        if (!icon.isNull()) {
            painter.drawPixmap(screenRect, icon, iconSource);
        }
        painter.setPen(Qt::white);
        painter.drawText(screenRect.adjusted(0, 0, 0, -2),
                         Qt::AlignHCenter | Qt::AlignBottom,
                         text);
    };
    drawInteractionArea(m_marketArea, QColor(255, 222, 126, 70),
                        QStringLiteral("SELL [E]"), m_marketIcon,
                        QRect(0, 0, 48, 48));
    drawInteractionArea(m_shopArea, QColor(114, 190, 255, 80),
                        QStringLiteral("SEEDS [E]"), m_marketIcon,
                        QRect(0, 0, 48, 48));
    drawInteractionArea(m_bedArea, QColor(180, 130, 220, 80),
                        QStringLiteral("SLEEP [E]"), m_bedIcon,
                        QRect(0, 16, 32, 32));
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

        const HarvestResult preview = m_worldState.previewHarvest(tilePos);
        if (preview.isValid() && m_harvestHandler &&
            !m_harvestHandler(preview.type, preview.quantity)) {
            emit harvestBlocked(preview.type, preview.quantity);
        } else if (preview.isValid()) {
            const HarvestResult harvested = m_worldState.harvest(tilePos);
            emit cropHarvested(harvested.type, harvested.quantity);
            emit worldChanged();
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
    emit worldChanged();
    return true;
}

bool MapWidget::isTileCultivatable(const QPoint &tilePos) const {
    return !m_marketArea.contains(tilePos) && !m_shopArea.contains(tilePos) &&
           !m_bedArea.contains(tilePos) &&
           m_worldState.isTileCultivatable(tilePos);
}

bool MapWidget::isPlantable(const QPoint &tilePos) const {
    return !m_marketArea.contains(tilePos) && !m_shopArea.contains(tilePos) &&
           !m_bedArea.contains(tilePos) &&
           m_worldState.isPlantable(tilePos);
}

bool MapWidget::plantSeed(const QPoint &tilePos, CropType cropType) {
    const qint64 gameMinute = m_getGameMinutes ? m_getGameMinutes() : 0;
    const int gameDay = static_cast<int>(gameMinute / (24 * 60));
    if (!isPlantable(tilePos)) {
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

bool MapWidget::waterTile(const QPoint &tilePos) {
    if (m_marketArea.contains(tilePos) || m_shopArea.contains(tilePos) ||
        m_bedArea.contains(tilePos) ||
        !m_worldState.waterTile(tilePos)) {
        return false;
    }
    update();
    emit worldChanged();
    return true;
}

DayAdvanceResult MapWidget::advanceDay(int completedDay,
                                       std::function<int()> randomPercent) {
    const DayAdvanceResult result =
        m_worldState.advanceDay(completedDay, std::move(randomPercent));
    update();
    return result;
}
