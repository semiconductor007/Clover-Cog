#include "farm/cropdefinition.h"
#include "map/mapdata.h"
#include "map/maprenderer.h"
#include "farm/worldstate.h"

#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSet>
#include <QDebug>

namespace {

void require(bool condition, const QString &message) {
    if (!condition) qFatal("%s", qPrintable(message));
}

QByteArray readFile(const QString &path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), QStringLiteral("Cannot read %1").arg(path));
    return file.readAll();
}

QJsonObject readObject(const QString &path) {
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(readFile(path), &error);
    require(error.error == QJsonParseError::NoError && doc.isObject(),
            QStringLiteral("Invalid JSON object: %1").arg(path));
    return doc.object();
}

QColor parseColor(const QString &text) {
    const QColor color(text);
    require(text.size() == 7 && text.startsWith('#') && color.isValid(),
            QStringLiteral("Invalid RGB color: %1").arg(text));
    return color;
}

QHash<QRgb, QRgb> readPalette(const QJsonObject &object) {
    require(!object.isEmpty(), QStringLiteral("Missing season palette"));
    QHash<QRgb, QRgb> palette;
    for (auto it = object.begin(); it != object.end(); ++it)
        palette.insert(parseColor(it.key()).rgb(), parseColor(it.value().toString()).rgb());
    return palette;
}

// Preview-only reference for C/E: apply each map to the original texture once,
// then cache by season and tileset. Do not feed a themed texture back into it.
QPixmap previewTexture(const QPixmap &original, const QHash<QRgb, QRgb> &palette) {
    QImage image = original.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb before = row[x];
            if (qAlpha(before) == 0) continue;
            const auto replacement = palette.constFind(qRgb(qRed(before), qGreen(before), qBlue(before)));
            if (replacement == palette.cend()) continue;
            row[x] = qRgba(qRed(*replacement), qGreen(*replacement), qBlue(*replacement), qAlpha(before));
        }
    }
    return QPixmap::fromImage(image);
}

void validateTexture(const QImage &original, const QImage &themed,
                     const QHash<QRgb, QRgb> &palette) {
    require(original.size() == themed.size(), QStringLiteral("Tileset dimensions changed"));
    for (int y = 0; y < original.height(); ++y) {
        for (int x = 0; x < original.width(); ++x) {
            const QRgb before = original.pixel(x, y);
            const QRgb after = themed.pixel(x, y);
            require(qAlpha(before) == qAlpha(after), QStringLiteral("Tileset alpha changed"));
            if (qAlpha(before) == 0) continue;
            const QRgb expected = palette.value(qRgb(qRed(before), qGreen(before), qBlue(before)), before);
            require(qRed(after) == qRed(expected) && qGreen(after) == qGreen(expected) &&
                    qBlue(after) == qBlue(expected), QStringLiteral("Unexpected palette pixel"));
        }
    }
}

QHash<QString, QJsonObject> validateManifest(const QJsonObject &manifest, const MapData &map) {
    require(manifest.value("schemaVersion").toInt() == 1 && manifest.value("tileSize").toInt() == 16,
            QStringLiteral("Unsupported seasonal asset schema/tile size"));
    const auto size = manifest.value("mapSize").toArray();
    require(size.size() == 2 && map.mapWidth == size[0].toInt() && map.mapHeight == size[1].toInt(),
            QStringLiteral("Map size differs from the scenery placement contract"));
    QHash<QString, QJsonObject> textures;
    QSet<QRgb> sourceColors;
    for (const auto &value : manifest.value("textures").toArray()) {
        const auto texture = value.toObject();
        const auto path = texture.value("imageResource").toString();
        const auto source = texture.value("tilesetSource").toString();
        require(!textures.contains(source), QStringLiteral("Duplicate texture binding"));
        const auto bytes = readFile(path);
        require(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() ==
                    texture.value("sha256").toString().toLatin1(),
                QStringLiteral("Original texture changed; review palette: %1").arg(path));
        const QImage image(path);
        const auto expected = texture.value("size").toArray();
        require(!image.isNull() && expected.size() == 2 && image.width() == expected[0].toInt() &&
                    image.height() == expected[1].toInt(), QStringLiteral("Texture size mismatch: %1").arg(path));
        bool found = false;
        for (const auto &tileset : map.tilesets) {
            if (tileset.sourcePath != source) continue;
            found = tileset.texture.size() == image.size();
        }
        require(found, QStringLiteral("Texture not bound to the loaded map: %1").arg(source));
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                if (qAlpha(image.pixel(x, y))) sourceColors.insert(image.pixel(x, y) | 0xff000000u);
        textures.insert(source, texture);
    }
    const QStringList expectedSeasons{"spring", "summer", "autumn", "winter"};
    const auto seasons = manifest.value("seasons").toArray();
    require(seasons.size() == expectedSeasons.size(), QStringLiteral("Four seasons required"));
    for (int i = 0; i < seasons.size(); ++i) {
        const auto season = seasons[i].toObject();
        require(season.value("id").toString() == expectedSeasons[i], QStringLiteral("Unexpected season order/ID"));
        const auto palette = readPalette(season.value("palette").toObject());
        for (auto color = palette.cbegin(); color != palette.cend(); ++color)
            require(sourceColors.contains(color.key()), QStringLiteral("Palette has an unused source color"));
    }
    require(textures.size() == 7, QStringLiteral("Expected seven environment texture bindings"));
    return textures;
}

QHash<QString, QJsonObject> validateDecorations(const QJsonObject &object, const MapData &map,
                                              const QJsonArray &sourceLayers) {
    require(object.value("schemaVersion").toInt() == 1 && object.value("tileSize").toInt() == 16,
            QStringLiteral("Unsupported decoration schema"));
    QHash<QString, QJsonObject> patterns;
    for (const auto &value : object.value("patterns").toArray()) {
        const auto pattern = value.toObject();
        const auto id = pattern.value("id").toString();
        require(!id.isEmpty() && !patterns.contains(id), QStringLiteral("Invalid decoration ID"));
        require(pattern.value("size").toArray() == QJsonArray{16, 16}, QStringLiteral("Decoration must be one tile"));
        for (const auto &part : pattern.value("rects").toArray()) {
            const auto rect = part.toObject().value("rect").toArray();
            require(rect.size() == 4, QStringLiteral("Invalid decoration rectangle"));
            const QRect bounds(rect[0].toInt(), rect[1].toInt(), rect[2].toInt(), rect[3].toInt());
            require(bounds.width() > 0 && bounds.height() > 0 && QRect(0, 0, 16, 16).contains(bounds),
                    QStringLiteral("Decoration outside tile bounds"));
            parseColor(part.toObject().value("color").toString());
        }
        patterns.insert(id, pattern);
    }
    QSet<QPoint> positions;
    for (const auto &value : object.value("placements").toArray()) {
        const auto tile = value.toArray();
        require(tile.size() == 2, QStringLiteral("Invalid scenery position"));
        const QPoint pos(tile[0].toInt(), tile[1].toInt());
        require(QRect(0, 0, map.mapWidth, map.mapHeight).contains(pos) && !positions.contains(pos),
                QStringLiteral("Scenery outside map or duplicated"));
        require(map.collisionLayer[pos.y()][pos.x()] == 0 && map.cultivatableLayer[pos.y()][pos.x()] == 13,
                QStringLiteral("Scenery must be on accessible grass"));
        // The water background extends underneath land; it is not an obstacle.
        // Check the named foreground layers from TMJ, rather than all static layers.
        const QSet<QString> foreground{"paths", "cultivated", "item2", "item1", "buildings", "crop", "tree"};
        for (const auto &value : sourceLayers) {
            const auto layer = value.toObject();
            if (!foreground.contains(layer.value("name").toString())) continue;
            require(layer.value("data").toArray()[pos.y() * map.mapWidth + pos.x()].toInt() == 0,
                    QStringLiteral("Scenery overlaps existing map content"));
        }
        positions.insert(pos);
    }
    require(patterns.size() == 4 && positions.size() == 12, QStringLiteral("Expected four patterns and 12 sites"));
    return patterns;
}

void paintDecorations(QPainter &painter, const QJsonObject &pattern,
                      const QJsonArray &positions, const WorldState &world) {
    for (const auto &value : positions) {
        const auto tile = value.toArray();
        const QPoint pos(tile[0].toInt(), tile[1].toInt());
        // Scenery is visual only and must yield to the player's farming actions.
        if (world.cultivatedLayer()[pos.y()][pos.x()] != 0 || world.cropGidAt(pos) != 0) continue;
        for (const auto &value : pattern.value("rects").toArray()) {
            const auto part = value.toObject();
            const auto rect = part.value("rect").toArray();
            painter.fillRect(pos.x() * 16 + rect[0].toInt(), pos.y() * 16 + rect[1].toInt(),
                             rect[2].toInt(), rect[3].toInt(), parseColor(part.value("color").toString()));
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QCommandLineParser args;
    args.addHelpOption();
    args.addOption({"output", "Output directory for map renders", "directory", "season-preview-output"});
    args.addOption({"font-file", "Optional local Chinese font file", "path"});
    args.process(app);
    if (args.isSet("font-file"))
        require(QFontDatabase::addApplicationFont(args.value("font-file")) >= 0, QStringLiteral("Cannot load requested font"));
    const QDir output(args.value("output"));
    require(QDir().mkpath(output.absolutePath()), QStringLiteral("Cannot create output directory"));
    const auto manifest = readObject(QStringLiteral(":/seasons/manifest.json"));
    MapData map;
    QString error;
    require(MapLoader::load(manifest.value("mapResource").toString(), &map, &error), error);
    const auto textures = validateManifest(manifest, map);
    const auto decorations = readObject(manifest.value("decorationsResource").toString());
    const auto patterns = validateDecorations(decorations, map,
        readObject(manifest.value("mapResource").toString()).value("layers").toArray());
    WorldState world;
    world.initialize(map);
    // Display samples only: use mature plants to verify that seasonal palettes
    // leave all four crop textures intact. No game files or saves are written.
    const auto types = CropCatalog::cropTypes();
    for (int i = 0; i < types.size(); ++i) {
        const QPoint tile(51 + i, 13);
        require(world.isPlantable(tile) || world.cultivate(tile), QStringLiteral("Sample crop field unavailable"));
    }
    auto snapshot = world.snapshot();
    for (int i = 0; i < types.size(); ++i)
        snapshot.crops.append({{51 + i, 13}, types[i], 0, CropCatalog::definition(types[i]).growthDays});
    require(world.restore(snapshot), QStringLiteral("Cannot restore display-only crop samples"));

    const int panelW = map.mapWidth * 16;
    const int panelH = map.mapHeight * 16;
    QImage overview(panelW * 2 + 72, panelH * 2 + 240, QImage::Format_ARGB32_Premultiplied);
    overview.fill(QColor("#f5f3e9"));
    QPainter overviewPainter(&overview);
    overviewPainter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    overviewPainter.setPen(QColor("#33482e"));
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPixelSize(30);
    overviewPainter.setFont(font);
    overviewPainter.drawText(24, 42, QStringLiteral("同一张地图的四季环境素材预览"));
    font.setPixelSize(17);
    overviewPainter.setFont(font);
    overviewPainter.drawText(24, 73, QStringLiteral("A 配色与装饰资源 · 原图块 / 透明边缘 / 碰撞保持 · E 运行时接入待完成"));

    int index = 0;
    for (const auto &value : manifest.value("seasons").toArray()) {
        const auto season = value.toObject();
        const auto id = season.value("id").toString();
        const auto palette = readPalette(season.value("palette").toObject());
        auto themed = map.tilesets;
        for (int i = 0; i < themed.size(); ++i) {
            if (!textures.contains(themed[i].sourcePath)) {
                require(themed[i].texture.toImage() == map.tilesets[i].texture.toImage(),
                        QStringLiteral("Unbound texture changed"));
                continue;
            }
            themed[i].texture = previewTexture(map.tilesets[i].texture, palette);
            validateTexture(map.tilesets[i].texture.toImage().convertToFormat(QImage::Format_ARGB32),
                            themed[i].texture.toImage().convertToFormat(QImage::Format_ARGB32), palette);
            require(themed[i].firstGid == map.tilesets[i].firstGid && themed[i].tilesPerRow == map.tilesets[i].tilesPerRow,
                    QStringLiteral("Tile layout changed"));
        }
        const QString patternId = season.value("decoration").toString();
        require(patterns.contains(patternId), QStringLiteral("Missing season decoration"));
        QImage image(panelW, panelH, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            MapRenderer renderer;
            renderer.paint(painter, image.rect(), {0, 0}, map.mapWidth, map.mapHeight, 16, 16,
                           themed, map.renderLayers, world);
            paintDecorations(painter, patterns.value(patternId), decorations.value("placements").toArray(), world);
        }
        const auto large = image.scaled(panelW * 2, panelH * 2, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        require(large.save(output.filePath(id + QStringLiteral("-map.png"))), QStringLiteral("Cannot save map render"));
        const int x = 24 + (index % 2) * (panelW + 24);
        const int y = 122 + (index / 2) * (panelH + 58);
        font.setPixelSize(23);
        overviewPainter.setFont(font);
        overviewPainter.drawText(x, y - 10, season.value("name").toString() + QStringLiteral(" / ") + id);
        overviewPainter.drawImage(x, y, image);
        ++index;
        qInfo().noquote() << "PASS:" << id << "seven textures retain layout/alpha; decoration bounds and placement valid";
    }
    font.setPixelSize(17);
    overviewPainter.setFont(font);
    overviewPainter.drawText(24, overview.height() - 25,
                             QStringLiteral("作物为展示样本；原图集直接渲染。此图验证资源效果，不代表游戏已自动换季。"));
    overviewPainter.end();
    require(overview.save(output.filePath(QStringLiteral("seasons-overview.png"))), QStringLiteral("Cannot save overview"));
    qInfo().noquote() << "PASS: four seasons; seven source hashes; 28 themed textures; 12 scenery sites; actual map renderer";
    return 0;
}
