#include "maprenderer.h"

#include <QPainter>
#include <QtGlobal>
#include <QFileInfo>

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
        if (renderLayer.role == MapLayerRole::Cultivated) {
            drawCultivatedLayer(painter, visibleTiles, offset, tileWidth, tileHeight, tilesets, worldState);
        } else {
            drawLayer(painter, resolveLayer(renderLayer, worldState), visibleTiles,
                      offset, tileWidth, tileHeight, tilesets);
        }
    }

    drawLayer(painter, worldState.cropLayer(), visibleTiles, offset,
              tileWidth, tileHeight, tilesets);
}

void MapRenderer::drawCultivatedLayer(QPainter &painter, const QRect &visible,
                                    const QPoint &offset, int tileWidth, int tileHeight,
                                    const QVector<Tileset> &tilesets, const WorldState &world) const {
    const auto &layer = world.cultivatedLayer();
    const Tileset *soil = nullptr;
    for (const auto &tileset : tilesets) {
        if (QFileInfo(tileset.sourcePath).fileName() == QStringLiteral("Tilled_Dirt_Wide_v2.tsx") &&
            tileset.tileWidth == 16 && tileset.tileHeight == 16 && tileset.tilesPerRow == 11 &&
            tileset.texture.width() >= 48 && tileset.texture.height() >= 48) {
            soil = &tileset;
            break;
        }
    }
    if (!soil) {
        drawLayer(painter, layer, visible, offset, tileWidth, tileHeight, tilesets);
        drawWateredSoil(painter, visible, offset, tileWidth, tileHeight, world);
        return;
    }
    const auto isSoil = [&](QPoint tile) {
        if (tile.y() < 0 || tile.y() >= layer.size() || tile.x() < 0 || tile.x() >= layer.at(tile.y()).size())
            return false;
        const int local = layer.at(tile.y()).at(tile.x()) - soil->firstGid;
        return local == 0 || local == 1 || local == 2 || local == 11 || local == 12 ||
               local == 13 || local == 22 || local == 23 || local == 24;
    };
    if (m_soilTextureKey != soil->texture.cacheKey()) {
        m_wetSoilTexture = soil->texture;
        QPainter wet(&m_wetSoilTexture);
        wet.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        wet.fillRect(m_wetSoilTexture.rect(), QColor(64, 92, 112, 105));
        wet.end();
        m_soilTextureKey = soil->texture.cacheKey();
    }
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int y = visible.top(); y <= visible.bottom(); ++y) {
        for (int x = visible.left(); x <= visible.right(); ++x) {
            const QPoint tile(x,y);
            if (y < 0 || y >= layer.size() || x < 0 || x >= layer.at(y).size()) continue;
            if (!isSoil(tile)) {
                if (layer.at(y).at(x)) drawTile(painter, layer.at(y).at(x), tile, offset, tileWidth, tileHeight, tilesets);
                continue;
            }
            const bool north = isSoil(tile + QPoint(0,-1)), east = isSoil(tile + QPoint(1,0));
            const bool south = isSoil(tile + QPoint(0,1)), west = isSoil(tile + QPoint(-1,0));
            const QPixmap &texture = world.isWatered(tile) ? m_wetSoilTexture : soil->texture;
            // Adapt the four-neighbour mask to this atlas's 3x3 border set.
            // Quarter composition also handles single cells and one-cell strips.
            for (int qy = 0; qy < 2; ++qy) for (int qx = 0; qx < 2; ++qx) {
                const bool joinedX = qx ? east : west, joinedY = qy ? south : north;
                const int sx = (joinedX ? 16 : (qx ? 32 : 0)) + qx * 8;
                const int sy = (joinedY ? 16 : (qy ? 32 : 0)) + qy * 8;
                const int left = x*tileWidth-offset.x() + qx*tileWidth/2;
                const int top = y*tileHeight-offset.y() + qy*tileHeight/2;
                const int right = x*tileWidth-offset.x() + (qx+1)*tileWidth/2;
                const int bottom = y*tileHeight-offset.y() + (qy+1)*tileHeight/2;
                painter.drawPixmap(QRect(left, top, right-left, bottom-top), texture, QRect(sx,sy,8,8));
            }
        }
    }
    painter.restore();
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
