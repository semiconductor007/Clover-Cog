#include "maprenderer.h"

#include <QPainter>
#include <QtGlobal>

void MapRenderer::paint(QPainter &painter, const QRect &viewport,
                        const QPoint &offset, int mapWidth, int mapHeight,
                        int tileWidth, int tileHeight,
                        const QVector<Tileset> &tilesets,
                        const QVector<MapRenderLayer> &renderLayers,
                        const WorldState &worldState) const {
    if (mapWidth <= 0 || mapHeight <= 0 || tileWidth <= 0 || tileHeight <= 0) {
        return;
    }

    const int startX = qBound(0, offset.x() / tileWidth, mapWidth);
    const int startY = qBound(0, offset.y() / tileHeight, mapHeight);
    const int endX = qBound(0, (offset.x() + viewport.width()) / tileWidth + 1,
                            mapWidth);
    const int endY = qBound(0, (offset.y() + viewport.height()) / tileHeight + 1,
                            mapHeight);
    const QRect visibleTiles(QPoint(startX, startY), QPoint(endX - 1, endY - 1));

    for (const MapRenderLayer &renderLayer : renderLayers) {
        drawLayer(painter, resolveLayer(renderLayer, worldState), visibleTiles,
                  offset, tileWidth, tileHeight, tilesets);
        if (renderLayer.role == MapLayerRole::Cultivated) {
            drawWateredSoil(painter, visibleTiles, offset, tileWidth,
                            tileHeight, worldState);
        }
    }

    drawLayer(painter, worldState.cropLayer(), visibleTiles, offset,
              tileWidth, tileHeight, tilesets);
}

void MapRenderer::drawWateredSoil(QPainter &painter, const QRect &visibleTiles,
                                  const QPoint &offset, int tileWidth,
                                  int tileHeight, const WorldState &worldState) {
    const QColor wetTint(64, 92, 112, 105);
    for (int y = visibleTiles.top(); y <= visibleTiles.bottom(); ++y) {
        for (int x = visibleTiles.left(); x <= visibleTiles.right(); ++x) {
            const QPoint tilePos(x, y);
            if (!worldState.isWatered(tilePos)) {
                continue;
            }
            painter.fillRect(x * tileWidth - offset.x(),
                             y * tileHeight - offset.y(),
                             tileWidth, tileHeight, wetTint);
        }
    }
}

const QVector<QVector<int>> &MapRenderer::resolveLayer(
    const MapRenderLayer &layer, const WorldState &worldState) {
    switch (layer.role) {
    case MapLayerRole::Cultivatable:
        return worldState.cultivatableLayer();
    case MapLayerRole::Cultivated:
        return worldState.cultivatedLayer();
    case MapLayerRole::Tree:
        return worldState.treeLayer();
    case MapLayerRole::Static:
        return layer.tiles;
    }
    return layer.tiles;
}

void MapRenderer::drawLayer(QPainter &painter,
                            const QVector<QVector<int>> &layer,
                            const QRect &visibleTiles, const QPoint &offset,
                            int tileWidth, int tileHeight,
                            const QVector<Tileset> &tilesets) {
    if (layer.isEmpty() || visibleTiles.isEmpty()) {
        return;
    }

    const int startY = qMax(0, visibleTiles.top());
    const int endY = qMin(visibleTiles.bottom() + 1, layer.size());
    for (int y = startY; y < endY; ++y) {
        const int startX = qMax(0, visibleTiles.left());
        const int endX = qMin(visibleTiles.right() + 1, layer.at(y).size());
        for (int x = startX; x < endX; ++x) {
            const int gid = layer.at(y).at(x);
            if (gid != 0) {
                drawTile(painter, gid, QPoint(x, y), offset,
                         tileWidth, tileHeight, tilesets);
            }
        }
    }
}

void MapRenderer::drawTile(QPainter &painter, int gid, const QPoint &tilePos,
                           const QPoint &offset, int tileWidth, int tileHeight,
                           const QVector<Tileset> &tilesets) {
    for (int i = tilesets.size() - 1; i >= 0; --i) {
        const Tileset &tileset = tilesets.at(i);
        if (gid < tileset.firstGid || tileset.tilesPerRow <= 0 ||
            tileset.tileWidth <= 0 || tileset.tileHeight <= 0) {
            continue;
        }

        const int localId = gid - tileset.firstGid;
        const int rows = tileset.texture.height() / tileset.tileHeight;
        if (localId >= tileset.tilesPerRow * rows) {
            continue;
        }

        const QRect source((localId % tileset.tilesPerRow) * tileset.tileWidth,
                           (localId / tileset.tilesPerRow) * tileset.tileHeight,
                           tileset.tileWidth, tileset.tileHeight);
        const QRect target(tilePos.x() * tileWidth - offset.x(),
                           tilePos.y() * tileHeight - offset.y(),
                           tileWidth, tileHeight);
        painter.drawPixmap(target, tileset.texture, source);
        return;
    }
}
