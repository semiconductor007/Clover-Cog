#include "map/seasonappearance.h"
#include "farm/worldstate.h"

#include <QPainter>
#include <QtTest>
#include <algorithm>

class SeasonAppearanceTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void appliesResourcePaletteAndCachesOriginalTilesets();
    void decorationsFollowCameraAndYieldToCultivation();

private:
    MapData m_map;
    SeasonAppearance m_appearance;
};

void SeasonAppearanceTests::initTestCase() {
    QString error;
    QVERIFY2(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &m_map, &error), qPrintable(error));
    QVERIFY(m_appearance.load(m_map.tilesets));
}

void SeasonAppearanceTests::appliesResourcePaletteAndCachesOriginalTilesets() {
    const auto grass = std::find_if(m_map.tilesets.cbegin(), m_map.tilesets.cend(),
        [](const Tileset &t) { return t.sourcePath == QStringLiteral("tsx/Grass.tsx"); });
    const auto water = std::find_if(m_map.tilesets.cbegin(), m_map.tilesets.cend(),
        [](const Tileset &t) { return t.sourcePath == QStringLiteral("Water.tsx"); });
    QVERIFY(grass != m_map.tilesets.cend());
    QVERIFY(water != m_map.tilesets.cend());
    const auto grassIndex = grass - m_map.tilesets.cbegin();
    const auto waterIndex = water - m_map.tilesets.cbegin();

    const auto original = grass->texture.toImage().convertToFormat(QImage::Format_ARGB32);
    QPoint sample(-1, -1);
    for (int y = 0; y < original.height() && sample.x() < 0; ++y) {
        for (int x = 0; x < original.width(); ++x) {
            if (original.pixel(x, y) == qRgb(0xc0, 0xd4, 0x70)) {
                sample = {x, y};
                break;
            }
        }
    }
    QVERIFY(sample.x() >= 0);
    const auto summerKey = m_appearance.tilesets(Season::Summer).at(grassIndex).texture.cacheKey();
    const QVector<QColor> grassColors{"#c0d470", "#9fc874", "#d9ba78", "#dce6df"};
    for (int i = 0; i < SeasonCalendar::SeasonsPerYear; ++i) {
        const auto season = SeasonCalendar::dateForDay(i * SeasonCalendar::DaysPerSeason).season;
        const auto tilesets = m_appearance.tilesets(season);
        QCOMPARE(tilesets.size(), m_map.tilesets.size());
        const auto &themed = tilesets.at(grassIndex);
        QCOMPARE(themed.texture.size(), grass->texture.size());
        QCOMPARE(themed.firstGid, grass->firstGid);
        QCOMPARE(themed.tilesPerRow, grass->tilesPerRow);
        QCOMPARE(themed.texture.toImage().pixelColor(sample), grassColors.at(i));
        QCOMPARE(tilesets.at(waterIndex).texture.cacheKey(), water->texture.cacheKey());
    }
    QCOMPARE(m_appearance.tilesets(Season::Summer).at(grassIndex).texture.cacheKey(), summerKey);
    QCOMPARE(grass->texture.toImage().pixelColor(sample), grassColors.first());
}

void SeasonAppearanceTests::decorationsFollowCameraAndYieldToCultivation() {
    WorldState world;
    world.initialize(m_map);
    const QPoint tile(14, 27);
    const QSize size(m_map.tileWidth, m_map.tileHeight);
    const QPoint camera(tile.x() * size.width(), tile.y() * size.height());
    const auto drawTile = [&](Season season) {
        QImage frame(size, QImage::Format_ARGB32_Premultiplied);
        frame.fill(Qt::transparent);
        QPainter painter(&frame);
        m_appearance.paintDecorations(season, painter, camera, world);
        painter.end();
        return frame;
    };
    QImage empty(size, QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);
    const auto flowers = drawTile(Season::Spring);
    QVERIFY(flowers != empty);
    const auto snow = drawTile(Season::Winter);
    QVERIFY(snow != empty);
    QVERIFY(snow != flowers);
    QVERIFY(world.cultivate(tile));
    QCOMPARE(drawTile(Season::Winter), empty);
    QVERIFY(world.plantSeed(tile, CropType::Wheat, 0));
    QVERIFY(world.fertilize(tile).success);
    QVERIFY(world.waterTile(tile));
    QCOMPARE(drawTile(Season::Spring), empty);
    QVERIFY(world.isWatered(tile));
    QVERIFY(world.hasPendingFertilizerBonus(tile));
}

QTEST_MAIN(SeasonAppearanceTests)
#include "seasonappearance_tests.moc"
