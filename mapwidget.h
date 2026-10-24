#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "mapdata.h"
#include "maprenderer.h"
#include "worldstate.h"

#include <QPoint>
#include <QPixmap>
#include <QRect>
#include <QVector>
#include <QWidget>
#include <functional>
#include <utility>

class MapWidget : public QWidget {
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr);

    bool loadMap(const QString &tmjPath);
    int tileWidth() const { return m_tileWidth; }
    int tileHeight() const { return m_tileHeight; }
    int mapWidth() const { return m_mapWidth; }
    int mapHeight() const { return m_mapHeight; }
    int offsetX() const { return m_offsetX; }
    int offsetY() const { return m_offsetY; }
    int viewportWidth() const { return width(); }
    int viewportHeight() const { return height(); }
    const QVector<QVector<int>> &collisionLayer() const { return m_worldState.collisionLayer(); }

    void setViewportCenter(int x, int y);
    void setTimeReference(std::function<qint64()> timeGetter) {
        m_getGameMinutes = std::move(timeGetter);
    }
    void setHarvestHandler(std::function<bool(CropType, int)> handler) {
        m_harvestHandler = std::move(handler);
    }
    void setPlantHandler(
        std::function<bool(CropType, const std::function<bool()> &)> handler) {
        m_plantHandler = std::move(handler);
    }
    void setMarketArea(const QRect &marketArea) {
        m_marketArea = marketArea;
        update();
    }
    void setShopArea(const QRect &shopArea) {
        m_shopArea = shopArea;
        update();
    }
    void setBedArea(const QRect &bedArea) {
        m_bedArea = bedArea;
        update();
    }
    WorldStateSnapshot worldSnapshot() const { return m_worldState.snapshot(); }
    bool restoreWorld(const WorldStateSnapshot &snapshot) {
        const bool restored = m_worldState.restore(snapshot);
        if (restored) update();
        return restored;
    }

    bool hasTreeAt(const QPoint &tilePos) const;
    bool modifyTreeHealth(const QPoint &tilePos, int damage);
    int tileAt(int x, int y) const;
    bool cultivateTile(const QPoint &tilePos);
    bool isTileCultivatable(const QPoint &tilePos) const;
    bool isPlantable(const QPoint &tilePos) const;
    bool plantSeed(const QPoint &tilePos, CropType cropType);
    bool waterTile(const QPoint &tilePos);
    DayAdvanceResult advanceDay(int completedDay,
                                std::function<int()> randomPercent = {});
    QPoint mapPointToTile(const QPoint &mapPoint) const;
    bool isValidTile(const QPoint &tilePos) const;

signals:
    void mapClicked(QPoint pos);
    void cropHarvested(CropType type, int quantity);
    void harvestBlocked(CropType type, int requiredCapacity);
    void plantingBlocked(CropType type);
    void mapLoadFailed(const QString &message);
    void worldChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    std::function<qint64()> m_getGameMinutes;
    std::function<bool(CropType, int)> m_harvestHandler;
    std::function<bool(CropType, const std::function<bool()> &)> m_plantHandler;
    int m_offsetX = 0;
    int m_offsetY = 0;
    QVector<MapRenderLayer> m_renderLayers;
    int m_tileWidth = 0;
    int m_tileHeight = 0;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    QVector<Tileset> m_tilesets;
    WorldState m_worldState;
    MapRenderer m_renderer;
    QRect m_marketArea;
    QRect m_shopArea;
    QRect m_bedArea;
    QPixmap m_marketIcon;
    QPixmap m_bedIcon;
};

#endif // MAPWIDGET_H
