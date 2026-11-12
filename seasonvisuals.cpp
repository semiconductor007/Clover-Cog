#include "seasonvisuals.h"

#include <QDataStream>
#include <QFileInfo>
#include <QImage>
#include <QIODevice>
#include <cmath>

bool SeasonVisualProfile::isValid() const {
    return static_cast<int>(season) >= 0 && static_cast<int>(season) < 4 &&
        leafHue >= -1 && leafHue < 360 && std::isfinite(saturationScale) &&
        std::isfinite(valueScale) && std::isfinite(valueLift) &&
        saturationScale >= 0 && saturationScale <= 2 && valueScale >= 0 &&
        valueScale <= 2 && valueLift >= 0 && valueLift <= 1 &&
        0 < dawnStart && dawnStart < dawnMiddle && dawnMiddle < dawnEnd &&
        dawnEnd < duskStart && duskStart < duskMiddle && duskMiddle < duskEnd &&
        duskEnd < 1440 && nightTint.isValid() && twilightTint.isValid() &&
        nightTint.alpha() <= 150 && twilightTint.alpha() <= 150;
}

QString SeasonVisualProfile::cacheKey() const {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << static_cast<int>(season) << leafHue << saturationScale << valueScale
           << valueLift << dawnStart << dawnMiddle << dawnEnd << duskStart
           << duskMiddle << duskEnd << nightTint << twilightTint << textureOverrides;
    return QString::fromLatin1(bytes.toHex());
}

SeasonVisualProfile SeasonVisuals::profile(Season season) {
    SeasonVisualProfile result;
    result.season = season;
    switch (season) {
    case Season::Spring: break;
    case Season::Summer:
        result.leafHue = 98; result.saturationScale = 1.12; result.valueScale = 0.95;
        result.dawnStart = 240; result.dawnMiddle = 300; result.dawnEnd = 360;
        result.duskStart = 1080; result.duskMiddle = 1200; result.duskEnd = 1320;
        result.nightTint = QColor(19, 34, 62, 128);
        break;
    case Season::Autumn:
        result.leafHue = 32; result.saturationScale = 1.12; result.valueScale = 0.98;
        result.dawnStart = 330; result.dawnMiddle = 390; result.dawnEnd = 450;
        result.duskStart = 960; result.duskMiddle = 1080; result.duskEnd = 1200;
        result.nightTint = QColor(34, 29, 58, 132);
        break;
    case Season::Winter:
        result.leafHue = 195; result.saturationScale = 0.25;
        result.valueScale = 0.85; result.valueLift = 0.14;
        result.dawnStart = 360; result.dawnMiddle = 420; result.dawnEnd = 480;
        result.duskStart = 900; result.duskMiddle = 1020; result.duskEnd = 1140;
        result.nightTint = QColor(20, 30, 58, 125);
        result.twilightTint = QColor(134, 124, 156, 30);
        break;
    }
    return result;
}

QStringList SeasonVisuals::vegetationAtlases() {
    return {QStringLiteral("Grass.tsx"), QStringLiteral("Grass_tiles_v2.tsx"),
            QStringLiteral("Basic_Grass_Biom_things.tsx"),
            QStringLiteral("Trees, stumps and bushes.tsx")};
}

bool SeasonVisuals::build(const QVector<Tileset> &base, const SeasonVisualProfile &profile,
                         QVector<Tileset> *result, QString *error) {
    const auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    if (!result || !profile.isValid() || base.isEmpty())
        return fail(QStringLiteral("Invalid season profile or unloaded map"));
    for (auto it = profile.textureOverrides.cbegin(); it != profile.textureOverrides.cend(); ++it) {
        bool found = false;
        for (const auto &atlas : base)
            found |= QFileInfo(atlas.sourcePath).fileName() == it.key();
        if (!found || it.value().isEmpty())
            return fail(QStringLiteral("Unknown atlas or empty override: %1").arg(it.key()));
    }
    QVector<Tileset> candidate = base; // Original resources are immutable.
    const QStringList vegetation = vegetationAtlases();
    for (auto &atlas : candidate) {
        const QString name = QFileInfo(atlas.sourcePath).fileName();
        const bool override = profile.textureOverrides.contains(name);
        if (!override && (profile.leafHue < 0 || !vegetation.contains(name))) continue;
        const QImage original = atlas.texture.toImage().convertToFormat(QImage::Format_ARGB32);
        if (original.isNull() || atlas.tileWidth <= 0 || atlas.tileHeight <= 0 ||
            original.width() % atlas.tileWidth || original.height() % atlas.tileHeight)
            return fail(QStringLiteral("Invalid source grid: %1").arg(name));
        QImage replacement = override ? QImage(profile.textureOverrides.value(name)) : original;
        if (replacement.isNull() || replacement.size() != original.size())
            return fail(QStringLiteral("Missing, unreadable or mismatched season image: %1").arg(name));
        replacement = replacement.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < original.height(); ++y) {
            const auto *source = reinterpret_cast<const QRgb *>(original.constScanLine(y));
            auto *target = reinterpret_cast<QRgb *>(replacement.scanLine(y));
            for (int x = 0; x < original.width(); ++x) {
                if (override) {
                    if (qAlpha(source[x]) != qAlpha(target[x]))
                        return fail(QStringLiteral("Season image alpha/grid layout differs: %1").arg(name));
                    continue;
                }
                const QColor color(source[x]);
                const int hue = color.hsvHue();
                // Select green vegetation-like pixels. Non-green details and
                // all non-environment atlases stay unchanged; no semantic art
                // classification is inferred for green details in these sheets.
                if (qAlpha(source[x]) == 0 || hue < 55 || hue > 170 || color.hsvSaturationF() < 0.16)
                    continue;
                QColor recolored = QColor::fromHsvF(profile.leafHue / 360.0,
                    qBound(0.0, color.hsvSaturationF() * profile.saturationScale, 1.0),
                    qBound(0.0, color.valueF() * profile.valueScale + profile.valueLift, 1.0),
                    qAlpha(source[x]) / 255.0);
                target[x] = recolored.rgba();
            }
        }
        atlas.texture = QPixmap::fromImage(replacement);
    }
    *result = candidate; // Commit only when every replacement has passed.
    if (error) error->clear();
    return true;
}
