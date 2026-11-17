#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "mapdata.h"
#include "maprenderer.h"
#include "farm/worldstate.h"

#include <QPoint>
#include <QPointer>
#include <QPixmap>
#include <QRect>
#include <QVector>
#include <QWidget>
#include <QElapsedTimer>
#include <QTimer>
#include <functional>
#include <utility>

class PlayerController;

class MapWidget : public QWidget {
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr);

    bool loadMap(const QString &tmjPath);
    void setPlayerController(PlayerController *controller);
    int tileWidth() const { return m_tileWidth; }
    int tileHeight() const { return m_tileHeight; }
    int mapWidth() const { return m_mapWidth; }
    int mapHeight() const { return m_mapHeight; }
    int offsetX() const { return m_offsetX; }
    int offsetY() const { return m_offsetY; }
    int viewportWidth() const { return width(); }
    int viewportHeight() const { return height(); }
    const QVector<QVector<int>> &collisionLayer() const { return m_worldState.collisionLayer(); }

    void setInputBlocked(bool blocked) { m_inputBlocked = blocked; }
    bool isInputBlocked() const { return m_inputBlocked; }
    // Presentation only: opening completion lets the window show the inventory.
    void setStorageChestOpen(bool open);
    void resetStorageChest();
    int storageChestFrame() const { return m_chestFrame; }
    const QVector<Tileset> &originalTilesets() const { return m_originalTilesets; }
    void setSeasonAppearance(QVector<Tileset> displayTilesets,
                             std::function<void(QPainter &, const QPoint &, const WorldState &)> paintScenery);
    void setFertilizeHandler(std::function<bool(const std::function<bool()> &)> handler) {
        m_fertilizeHandler = std::move(handler);
    }
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
    void setStorageArea(const QRect &storageArea) {
        m_storageArea = storageArea;
        update();
    }
    void setBedArea(const QRect &bedArea) {
        m_bedArea = bedArea;
        update();
    }
    WorldStateSnapshot worldSnapshot() const { return m_worldState.snapshot(); }
    bool canRestoreWorld(const WorldStateSnapshot &snapshot) const {
        auto candidate=m_worldState;return candidate.restore(snapshot);
    }
    bool restoreWorld(const WorldStateSnapshot &snapshot) {
        const bool restored = m_worldState.restore(snapshot);
        if (restored) {m_treeVisuals.clear(); update();}
        return restored;
    }

    bool hasTreeAt(const QPoint &tilePos) const;
    bool modifyTreeHealth(const QPoint &tilePos, int damage);
    bool chopTree(const QPoint &tilePos);
    void setWoodHandler(std::function<bool(int,const std::function<bool()> &)> handler) {m_woodHandler=std::move(handler);}

    int tileAt(int x, int y) const;
    bool cultivateTile(const QPoint &tilePos);
    bool isTileCultivatable(const QPoint &tilePos) const;
    bool isPlantable(const QPoint &tilePos) const;
    bool plantSeed(const QPoint &tilePos, CropType cropType);
    bool fertilizeTile(const QPoint &tilePos);
    bool waterTile(const QPoint &tilePos);
    DayAdvanceResult advanceDay(int completedDay,
                                std::function<int()> randomPercent = {});
    DayAdvanceResult advanceThroughDay(int completedDay);
    QPoint mapPointToTile(const QPoint &mapPoint) const;
    bool isValidTile(const QPoint &tilePos) const;

signals:
    void storageChestOpened();
    void storageChestCue(bool opening);
    void mapClicked(QPoint pos);
    void treeChopped(bool felled);
    void farmingFeedback(QString message);
    void tileWatered();
    void cropHarvested(CropType type, int quantity);
    void harvestBlocked(CropType type, int requiredCapacity);
    void plantingBlocked(CropType type);
    void plantingOutOfSeason(CropType type);
    void mapLoadFailed(const QString &message);
    void worldChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void advanceStorageChest();
    QTimer m_chestTimer;
    QElapsedTimer m_chestElapsed;
    bool m_chestTargetOpen = false;
    int m_chestFrame = 0;
    int m_chestStartFrame = 0;
    bool isInteractionTile(const QPoint &tilePos) const;
    std::function<bool(int,const std::function<bool()> &)> m_woodHandler;
    struct TreeVisual { QPoint anchor; QRect bounds; QPixmap texture; QElapsedTimer elapsed; bool falling=false; bool useFallSheet=false; bool fallRight=true; };
    QVector<TreeVisual> m_treeVisuals;
    QTimer m_treeVisualTimer;

    QPointer<PlayerController> m_playerController;
    std::function<qint64()> m_getGameMinutes;
    std::function<bool(CropType, int)> m_harvestHandler;
    std::function<bool(CropType, const std::function<bool()> &)> m_plantHandler;
    std::function<bool(const std::function<bool()> &)> m_fertilizeHandler;
    std::function<void(QPainter &, const QPoint &, const WorldState &)> m_paintScenery;
    bool m_inputBlocked = false;
    int m_offsetX = 0;
    int m_offsetY = 0;
    QVector<MapRenderLayer> m_renderLayers;
    int m_tileWidth = 0;
    int m_tileHeight = 0;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    QVector<Tileset> m_originalTilesets;
    QVector<Tileset> m_displayTilesets;
    WorldState m_worldState;
    MapRenderer m_renderer;
    QRect m_marketArea;
    QRect m_shopArea;
    QRect m_storageArea;
    QRect m_bedArea;
};

#endif // MAPWIDGET_H
