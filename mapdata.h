#ifndef MAPDATA_H
#define MAPDATA_H

#include <QHash>
#include <QPixmap>
#include <QString>
#include <QVector>

struct Tileset {
    QPixmap texture;
    int firstGid = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    int tilesPerRow = 0;
    QString sourcePath;
};

struct TileProperties {
    int health = 0;
    bool interactable = false;
    bool cultivable = false;
    QString cropType;
    int growthStage = 0;
};

enum class MapLayerRole {
    Static,
    Cultivatable,
    Cultivated,
    Tree
};

struct MapRenderLayer {
    MapLayerRole role = MapLayerRole::Static;
    QVector<QVector<int>> tiles;
};

struct MapData {
    int tileWidth = 0;
    int tileHeight = 0;
    int mapWidth = 0;
    int mapHeight = 0;
    QVector<Tileset> tilesets;
    QVector<MapRenderLayer> renderLayers;
    QVector<QVector<int>> collisionLayer;
    QVector<QVector<int>> cultivatableLayer;
    QVector<QVector<int>> cultivatedLayer;
    QVector<QVector<int>> cropLayer;
    QVector<QVector<int>> treeLayer;
    QHash<int, TileProperties> tileProperties;
};

class MapLoader {
public:
    static bool load(const QString &tmjPath, MapData *result, QString *errorMessage = nullptr);
};

#endif // MAPDATA_H
