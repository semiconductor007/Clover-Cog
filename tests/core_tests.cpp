#include <QtTest>

#include "collisiondetector.h"
#include "gameclock.h"
#include "gameconstants.h"
#include "mapdata.h"
#include "worldstate.h"

#include <QTemporaryFile>

class CoreTests : public QObject {
    Q_OBJECT

private slots:
    void gameClockConvertsElapsedTime();
    void collisionDetectorHandlesBounds();
    void mapLoaderValidatesAndInitializesLayers();
    void mapLoaderPreservesDynamicLayerOrder();
    void worldStateHandlesFarmingAndTrees();
};

void CoreTests::gameClockConvertsElapsedTime() {
    constexpr int speed = 1000;
    QCOMPARE(GameClock::minuteFromElapsed(90 * 1000, speed), 30);
    QCOMPARE(GameClock::hourFromElapsed(90 * 1000, speed), 1);
    QCOMPARE(GameClock::dayFromElapsed(24LL * 60 * 1000, speed), 1);
}

void CoreTests::collisionDetectorHandlesBounds() {
    QVector<QVector<int>> layer(2, QVector<int>(2, 0));
    layer[1][1] = 1;
    CollisionDetector detector(&layer, 16, 16);

    QVERIFY(!detector.isColliding(QRect(0, 0, 8, 8)));
    QVERIFY(detector.isColliding(QRect(16, 16, 8, 8)));
    QVERIFY(!detector.isColliding(QRect(-32, -32, 8, 8)));
}

void CoreTests::mapLoaderValidatesAndInitializesLayers() {
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray json = R"({
        "tilewidth": 16,
        "tileheight": 16,
        "width": 2,
        "height": 2,
        "tilesets": [],
        "layers": [
            {"name":"ground","type":"tilelayer","width":2,"height":2,"data":[0,0,0,0]}
        ]
    })";
    QCOMPARE(file.write(json), json.size());
    file.flush();

    MapData data;
    QString error;
    QVERIFY2(MapLoader::load(file.fileName(), &data, &error), qPrintable(error));
    QCOMPARE(data.mapWidth, 2);
    QCOMPARE(data.mapHeight, 2);
    QCOMPARE(data.collisionLayer.size(), 2);
    QCOMPARE(data.cropLayer.size(), 2);
    QCOMPARE(data.renderLayers.size(), 1);
}

void CoreTests::mapLoaderPreservesDynamicLayerOrder() {
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray json = R"({
        "tilewidth": 16,
        "tileheight": 16,
        "width": 1,
        "height": 1,
        "tilesets": [],
        "layers": [
            {"name":"ground","type":"tilelayer","width":1,"height":1,"data":[1]},
            {"name":"cultivatable","type":"tilelayer","width":1,"height":1,"data":[13]},
            {"name":"cultivated","type":"tilelayer","width":1,"height":1,"data":[0]},
            {"name":"tree","type":"tilelayer","width":1,"height":1,"data":[7]},
            {"name":"crop","type":"tilelayer","width":1,"height":1,"data":[0]}
        ]
    })";
    QCOMPARE(file.write(json), json.size());
    file.flush();

    MapData data;
    QString error;
    QVERIFY2(MapLoader::load(file.fileName(), &data, &error), qPrintable(error));
    QCOMPARE(data.renderLayers.size(), 4);
    QCOMPARE(data.renderLayers.at(0).role, MapLayerRole::Static);
    QCOMPARE(data.renderLayers.at(1).role, MapLayerRole::Cultivatable);
    QCOMPARE(data.renderLayers.at(2).role, MapLayerRole::Cultivated);
    QCOMPARE(data.renderLayers.at(3).role, MapLayerRole::Tree);
    QCOMPARE(data.treeLayer.at(0).at(0), 7);
    QCOMPARE(data.cropLayer.at(0).at(0), 0);
}

void CoreTests::worldStateHandlesFarmingAndTrees() {
    MapData data;
    data.mapWidth = 2;
    data.mapHeight = 2;
    data.cultivatableLayer = {{GameConstants::CultivatableGrassGid, 0}, {0, 0}};
    data.cultivatedLayer = {{0, 0}, {123, 0}};
    data.cropLayer = {{0, 0}, {0, 0}};
    data.treeLayer = {{0, 7}, {0, 0}};
    data.collisionLayer = {{0, 1}, {0, 0}};
    TileProperties treeProperties;
    treeProperties.health = 30;
    treeProperties.interactable = true;
    data.tileProperties.insert(7, treeProperties);

    WorldState state;
    state.initialize(data);

    const QPoint field(0, 0);
    QVERIFY(state.isTileCultivatable(field));
    QVERIFY(state.cultivate(field));
    QVERIFY(state.isPlantable(field));
    QVERIFY(state.plantSeed(field, CropType::Wheat, 0));
    QCOMPARE(state.cropGidAt(field), GameConstants::CropFirstStageGid);
    QVERIFY(state.advanceGrowth(GameConstants::CropMinutesPerStage * 3));
    QCOMPARE(state.cropGidAt(field), GameConstants::CropMatureStageGid);
    QCOMPARE(state.harvest(field), CropType::Wheat);
    QCOMPARE(state.cropGidAt(field), 0);

    const QPoint alternateSoil(0, 1);
    QVERIFY(state.isPlantable(alternateSoil));
    QVERIFY(state.plantSeed(alternateSoil, CropType::Potato, 5));

    const QPoint tree(1, 0);
    CollisionDetector detector(&state.collisionLayer(), 16, 16);
    QVERIFY(state.hasTreeAt(tree));
    QVERIFY(detector.isColliding(QRect(16, 0, 8, 8)));
    QCOMPARE(state.treeGidAt(tree), 7);
    QVERIFY(state.damageTree(tree, 10));
    QVERIFY(state.hasTreeAt(tree));
    QCOMPARE(state.treeGidAt(tree), 7);
    QVERIFY(state.damageTree(tree, 20));
    QVERIFY(!state.hasTreeAt(tree));
    QCOMPARE(state.treeGidAt(tree), 0);
    QVERIFY(!detector.isColliding(QRect(16, 0, 8, 8)));

    QVERIFY(!state.cultivate(QPoint(-1, 0)));
    QVERIFY(!state.plantSeed(QPoint(2, 0), CropType::Potato, 0));
    QCOMPARE(state.harvest(QPoint(2, 0)), CropType::None);
}

QTEST_APPLESS_MAIN(CoreTests)

#include "core_tests.moc"
