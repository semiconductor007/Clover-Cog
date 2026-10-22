#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "mapdata.h"
#include "maprenderer.h"
#include "worldstate.h"

#include <QPoint>
#include <QTimer>
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

    bool hasTreeAt(const QPoint &tilePos) const;
    bool modifyTreeHealth(const QPoint &tilePos, int damage);
    int tileAt(int x, int y) const;
    bool cultivateTile(const QPoint &tilePos);
    bool isTileCultivatable(const QPoint &tilePos) const;
    bool isPlantable(const QPoint &tilePos) const;
    bool plantSeed(const QPoint &tilePos, CropType cropType);
    QPoint mapPointToTile(const QPoint &mapPoint) const;
    bool isValidTile(const QPoint &tilePos) const;

signals:
    void mapClicked(QPoint pos);
    void cropHarvested(CropType type);
    void mapLoadFailed(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void updateGrowthStages();

private:
    std::function<qint64()> m_getGameMinutes;
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
    QTimer *m_growthTimer = nullptr;
};

#endif // MAPWIDGET_H
