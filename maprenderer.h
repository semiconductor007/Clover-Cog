#ifndef MAPRENDERER_H
#define MAPRENDERER_H

#include "mapdata.h"
#include "worldstate.h"

#include <QPoint>
#include <QRect>

class QPainter;

class MapRenderer {
public:
    void paint(QPainter &painter, const QRect &viewport, const QPoint &offset,
               int mapWidth, int mapHeight, int tileWidth, int tileHeight,
               const QVector<Tileset> &tilesets,
               const QVector<MapRenderLayer> &renderLayers,
               const WorldState &worldState) const;

private:
    static const QVector<QVector<int>> &resolveLayer(const MapRenderLayer &layer,
                                                     const WorldState &worldState);
    static void drawLayer(QPainter &painter, const QVector<QVector<int>> &layer,
                          const QRect &visibleTiles, const QPoint &offset,
                          int tileWidth, int tileHeight,
                          const QVector<Tileset> &tilesets);
    static void drawTile(QPainter &painter, int gid, const QPoint &tilePos,
                         const QPoint &offset, int tileWidth, int tileHeight,
                         const QVector<Tileset> &tilesets);
};

#endif // MAPRENDERER_H
