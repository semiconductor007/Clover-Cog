#include "mapdata.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>
#include <utility>

namespace {

QVector<QVector<int>> emptyLayer(int width, int height) {
    return QVector<QVector<int>>(height, QVector<int>(width, 0));
}

void appendRenderLayer(MapData *mapData, MapLayerRole role,
                       const QVector<QVector<int>> &tiles) {
    MapRenderLayer layer;
    layer.role = role;
    layer.tiles = tiles;
    mapData->renderLayers.append(layer);
}

bool loadTileProperties(const QString &tsxPath, int firstGid,
                        QHash<int, TileProperties> *properties) {
    QFile file(tsxPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QStringLiteral("tile")) {
            continue;
        }

        const int globalId = firstGid + xml.attributes().value("id").toInt();
        TileProperties tileProperties;

        while (!(xml.isEndElement() && xml.name() == QStringLiteral("tile")) && !xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement() || xml.name() != QStringLiteral("property")) {
                continue;
            }

            const QString name = xml.attributes().value("name").toString();
            const QString value = xml.attributes().value("value").toString();
            if (name == QStringLiteral("health")) tileProperties.health = value.toInt();
            else if (name == QStringLiteral("interactable")) tileProperties.interactable = value == QStringLiteral("true");
            else if (name == QStringLiteral("type") && value == QStringLiteral("tree")) tileProperties.interactable = true;
            else if (name == QStringLiteral("cultivable")) tileProperties.cultivable = value == QStringLiteral("true");
            else if (name == QStringLiteral("cropType")) tileProperties.cropType = value;
            else if (name == QStringLiteral("growthStage")) tileProperties.growthStage = value.toInt();
        }

        properties->insert(globalId, tileProperties);
    }
    return !xml.hasError();
}

} // namespace

bool MapLoader::load(const QString &tmjPath, MapData *result, QString *errorMessage) {
    if (!result) {
        if (errorMessage) *errorMessage = QStringLiteral("MapData output is null");
        return false;
    }

    QFile file(tmjPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) *errorMessage = QStringLiteral("Unable to open map: %1").arg(tmjPath);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull() || !document.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid map JSON: %1").arg(parseError.errorString());
        return false;
    }

    MapData loaded;
    const QJsonObject map = document.object();
    loaded.tileWidth = map.value("tilewidth").toInt();
    loaded.tileHeight = map.value("tileheight").toInt();
    loaded.mapWidth = map.value("width").toInt();
    loaded.mapHeight = map.value("height").toInt();
    if (loaded.tileWidth <= 0 || loaded.tileHeight <= 0 ||
        loaded.mapWidth <= 0 || loaded.mapHeight <= 0) {
        if (errorMessage) *errorMessage = QStringLiteral("Map dimensions or tile dimensions are invalid");
        return false;
    }

    for (const QJsonValue &tilesetValue : map.value("tilesets").toArray()) {
        const QJsonObject source = tilesetValue.toObject();
        Tileset tileset;
        tileset.firstGid = source.value("firstgid").toInt();
        tileset.sourcePath = source.value("source").toString();

        const QString tsxName = QFileInfo(tileset.sourcePath).fileName();
        loadTileProperties(QStringLiteral(":/tilesets/tsx/") + tsxName,
                           tileset.firstGid, &loaded.tileProperties);

        const QString imageName = QString(tsxName).replace(QStringLiteral(".tsx"), QStringLiteral(".png"));
        const QString imagePath = QStringLiteral(":/tilesets/sprout/") + imageName;
        if (!tileset.texture.load(imagePath)) {
            if (errorMessage) *errorMessage = QStringLiteral("Unable to load tileset image: %1").arg(imagePath);
            return false;
        }
        tileset.tileWidth = loaded.tileWidth;
        tileset.tileHeight = loaded.tileHeight;
        tileset.tilesPerRow = tileset.texture.width() / loaded.tileWidth;
        if (tileset.tilesPerRow <= 0) {
            if (errorMessage) *errorMessage = QStringLiteral("Tileset has an invalid width: %1").arg(imagePath);
            return false;
        }
        loaded.tilesets.append(tileset);
    }

    const int expectedSize = loaded.mapWidth * loaded.mapHeight;
    for (const QJsonValue &layerValue : map.value("layers").toArray()) {
        const QJsonObject layer = layerValue.toObject();
        if (layer.value("type").toString() != QStringLiteral("tilelayer")) {
            continue;
        }
        const QString layerName = layer.value("name").toString();
        if (layer.value("width").toInt() != loaded.mapWidth ||
            layer.value("height").toInt() != loaded.mapHeight) {
            if (errorMessage) *errorMessage = QStringLiteral("Layer dimensions do not match the map: %1").arg(layerName);
            return false;
        }

        const QJsonArray values = layer.value("data").toArray();
        if (values.size() < expectedSize) {
            if (errorMessage) *errorMessage = QStringLiteral("Layer data is incomplete: %1").arg(layerName);
            return false;
        }

        QVector<QVector<int>> layerData = emptyLayer(loaded.mapWidth, loaded.mapHeight);
        for (int y = 0; y < loaded.mapHeight; ++y) {
            for (int x = 0; x < loaded.mapWidth; ++x) {
                layerData[y][x] = values.at(y * loaded.mapWidth + x).toInt();
            }
        }

        const QString lowerName = layerName.toLower();
        if (lowerName.contains(QStringLiteral("collision"))) {
            loaded.collisionLayer = layerData;
        } else if (lowerName == QStringLiteral("cultivatable")) {
            loaded.cultivatableLayer = layerData;
            appendRenderLayer(&loaded, MapLayerRole::Cultivatable, layerData);
        } else if (lowerName == QStringLiteral("cultivated")) {
            loaded.cultivatedLayer = layerData;
            appendRenderLayer(&loaded, MapLayerRole::Cultivated, layerData);
        } else if (lowerName == QStringLiteral("crop")) {
            loaded.cropLayer = layerData;
        } else if (lowerName == QStringLiteral("tree")) {
            loaded.treeLayer = layerData;
            appendRenderLayer(&loaded, MapLayerRole::Tree, layerData);
        } else {
            appendRenderLayer(&loaded, MapLayerRole::Static, layerData);
        }
    }

    if (loaded.collisionLayer.isEmpty()) loaded.collisionLayer = emptyLayer(loaded.mapWidth, loaded.mapHeight);
    if (loaded.cultivatableLayer.isEmpty()) loaded.cultivatableLayer = emptyLayer(loaded.mapWidth, loaded.mapHeight);
    if (loaded.cultivatedLayer.isEmpty()) loaded.cultivatedLayer = emptyLayer(loaded.mapWidth, loaded.mapHeight);
    if (loaded.cropLayer.isEmpty()) loaded.cropLayer = emptyLayer(loaded.mapWidth, loaded.mapHeight);

    *result = std::move(loaded);
    return true;
}
