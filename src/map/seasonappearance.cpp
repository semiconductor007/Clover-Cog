#include "seasonappearance.h"
#include "farm/worldstate.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

namespace {

QJsonObject readResource(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

QPixmap renderPattern(const QJsonObject &pattern, int tileSize) {
    QPixmap sprite(tileSize, tileSize);
    sprite.fill(Qt::transparent);
    QPainter painter(&sprite);
    for (const auto &entry : pattern.value("rects").toArray()) {
        const auto part = entry.toObject();
        const auto rect = part.value("rect").toArray();
        if (rect.size() == 4) {
            painter.fillRect(rect[0].toInt(), rect[1].toInt(), rect[2].toInt(),
                             rect[3].toInt(), QColor(part.value("color").toString()));
        }
    }
    painter.end();
    return sprite;
}

QPixmap recolor(const QPixmap &original, const QHash<QRgb, QRgb> &palette) {
    QImage image = original.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = row[x];
            if (!qAlpha(pixel)) continue;
            const auto color = palette.constFind(qRgb(qRed(pixel), qGreen(pixel), qBlue(pixel)));
            if (color != palette.cend()) {
                row[x] = qRgba(qRed(*color), qGreen(*color), qBlue(*color), qAlpha(pixel));
            }
        }
    }
    return QPixmap::fromImage(image);
}

} // namespace

bool SeasonAppearance::load(const QVector<Tileset> &originals) {
    // A cache belongs to this exact map and version of the resource bundle.
    m_originals = originals;
    m_textureSources.clear();
    m_styles.clear();
    m_placements.clear();
    const auto manifest = readResource(QStringLiteral(":/seasons/manifest.json"));
    const auto decorations = readResource(manifest.value("decorationsResource").toString());
    m_tileSize = manifest.value("tileSize").toInt();
    if (manifest.value("schemaVersion").toInt() != 1 || m_tileSize <= 0 ||
        decorations.value("schemaVersion").toInt() != 1 ||
        decorations.value("tileSize").toInt() != m_tileSize) {
        qWarning("Cannot load the seasonal map resources");
        return false;
    }
    for (const auto &entry : manifest.value("textures").toArray()) {
        m_textureSources.insert(entry.toObject().value("tilesetSource").toString());
    }
    for (const auto &entry : decorations.value("placements").toArray()) {
        const auto tile = entry.toArray();
        if (tile.size() == 2) m_placements.append({tile[0].toInt(), tile[1].toInt()});
    }
    QHash<QString, QPixmap> patterns;
    for (const auto &entry : decorations.value("patterns").toArray()) {
        const auto pattern = entry.toObject();
        patterns.insert(pattern.value("id").toString(), renderPattern(pattern, m_tileSize));
    }
    for (const auto &entry : manifest.value("seasons").toArray()) {
        const auto season = entry.toObject();
        Style style;
        style.decoration = patterns.value(season.value("decoration").toString());
        const auto palette = season.value("palette").toObject();
        for (auto it = palette.begin(); it != palette.end(); ++it) {
            const QRgb before = QColor(it.key()).rgb();
            const QRgb after = QColor(it.value().toString()).rgb();
            if (before != after) style.palette.insert(before, after);
        }
        m_styles.insert(season.value("id").toString(), style);
    }
    return true;
}

QVector<Tileset> SeasonAppearance::tilesets(Season season) {
    auto style = m_styles.find(SeasonCalendar::seasonId(season));
    if (style == m_styles.end() || style->palette.isEmpty()) return m_originals;
    if (style->tilesets.isEmpty()) {
        style->tilesets = m_originals;
        for (auto &tileset : style->tilesets) {
            if (m_textureSources.contains(tileset.sourcePath)) {
                tileset.texture = recolor(tileset.texture, style->palette);
            }
        }
    }
    return style->tilesets;
}

void SeasonAppearance::paintDecorations(Season season, QPainter &painter,
                                       const QPoint &offset, const WorldState &world) const {
    const auto style = m_styles.constFind(SeasonCalendar::seasonId(season));
    if (style == m_styles.cend() || style->decoration.isNull()) return;
    for (const auto &tile : m_placements) {
        if (world.isValidTile(tile) && world.cultivatedLayer().at(tile.y()).at(tile.x()) == 0 &&
            world.cropGidAt(tile) == 0) {
            painter.drawPixmap(tile * m_tileSize - offset, style->decoration);
        }
    }
}
