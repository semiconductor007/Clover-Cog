#ifndef SEASONAPPEARANCE_H
#define SEASONAPPEARANCE_H

#include "mapdata.h"
#include "seasoncalendar.h"

#include <QSet>

class QPainter;
class WorldState;

class SeasonAppearance {
public:
    bool load(const QVector<Tileset> &originals);
    QVector<Tileset> tilesets(Season season);
    void paintDecorations(Season season, QPainter &painter, const QPoint &offset,
                          const WorldState &world) const;

private:
    struct Style {
        QHash<QRgb, QRgb> palette;
        QPixmap decoration;
        QVector<Tileset> tilesets;
    };

    QVector<Tileset> m_originals;
    QSet<QString> m_textureSources;
    QHash<QString, Style> m_styles;
    QVector<QPoint> m_placements;
    int m_tileSize = 0;
};

#endif // SEASONAPPEARANCE_H
