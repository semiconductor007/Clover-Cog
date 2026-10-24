#include <QtTest>

#include "collisiondetector.h"
#include "cropdefinition.h"
#include "economyservice.h"
#include "gameclock.h"
#include "gameconstants.h"
#include "mapdata.h"
#include "savemanager.h"
#include "worldstate.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>

class CoreTests : public QObject {
    Q_OBJECT

private slots:
    void gameClockConvertsElapsedTime();
    void collisionDetectorHandlesBounds();
    void mapLoaderValidatesAndInitializesLayers();
    void mapLoaderPreservesDynamicLayerOrder();
    void worldStateHandlesFarmingAndTrees();
    void cropDefinitionsControlGrowthAndYield();
    void dailyWateringControlsGrowthAndSoilRecovery();
    void inventoryEnforcesWholeHarvestCapacity();
    void economyRequiresMarketAndSellsAtomically();
    void seedsAreConsumedAndPurchasedAtomically();
    void saveManagerRoundTripsGameStateAndKeepsBackup();
};

void CoreTests::gameClockConvertsElapsedTime() {
    constexpr int speed = 1000;
    QCOMPARE(GameClock::minuteFromElapsed(90 * 1000, speed), 30);
    QCOMPARE(GameClock::hourFromElapsed(90 * 1000, speed), 1);
    QCOMPARE(GameClock::dayFromElapsed(24LL * 60 * 1000, speed), 1);

    GameClock clock(1000000);
    clock.start();
    clock.sleepToNextMorning(6);
    QCOMPARE(clock.day(), 1);
    QCOMPARE(clock.hour(), 6);
    clock.setElapsedGameMinutes(2 * 24 * 60 + 13 * 60 + 25);
    QCOMPARE(clock.day(), 2);
    QCOMPARE(clock.hour(), 13);
    QCOMPARE(clock.minute(), 25);
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
    for (int day = 0; day < CropCatalog::definition(CropType::Wheat).growthDays;
         ++day) {
        QVERIFY(state.waterTile(field));
        state.advanceDay(day, []() { return 99; });
    }
    QCOMPARE(state.cropGidAt(field), GameConstants::CropMatureStageGid);
    const HarvestResult wheatHarvest = state.harvest(field);
    QCOMPARE(wheatHarvest.type, CropType::Wheat);
    QCOMPARE(wheatHarvest.quantity, 1);
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
    QVERIFY(!state.harvest(QPoint(2, 0)).isValid());
}

void CoreTests::cropDefinitionsControlGrowthAndYield() {
    const CropDefinition &wheat = CropCatalog::definition(CropType::Wheat);
    const CropDefinition &potato = CropCatalog::definition(CropType::Potato);
    QCOMPARE(wheat.growthDays, 3);
    QCOMPARE(wheat.harvestYield, 1);
    QCOMPARE(wheat.seedPrice, 2);
    QCOMPARE(wheat.sellPrice, 4);
    QCOMPARE(potato.growthDays, 5);
    QCOMPARE(potato.harvestYield, 3);
    QCOMPARE(potato.seedPrice, 5);
    QCOMPARE(potato.sellPrice, 3);

    MapData data;
    data.mapWidth = 1;
    data.mapHeight = 1;
    data.cultivatedLayer = {{GameConstants::CultivatedTileGid}};
    WorldState state;
    state.initialize(data);
    QVERIFY(state.plantSeed(QPoint(0, 0), CropType::Potato, 0));
    QVERIFY(state.waterTile(QPoint(0, 0)));
    state.advanceDay(0, []() { return 99; });
    QVERIFY(state.waterTile(QPoint(0, 0)));
    state.advanceDay(1, []() { return 99; });
    QCOMPARE(state.cropGidAt(QPoint(0, 0)), potato.stageGids.at(1));
    for (int day = 2; day < potato.growthDays; ++day) {
        QVERIFY(state.waterTile(QPoint(0, 0)));
        state.advanceDay(day, []() { return 99; });
    }
    const HarvestResult harvest = state.previewHarvest(QPoint(0, 0));
    QCOMPARE(harvest.type, CropType::Potato);
    QCOMPARE(harvest.quantity, 3);
}

void CoreTests::dailyWateringControlsGrowthAndSoilRecovery() {
    MapData data;
    data.mapWidth = 2;
    data.mapHeight = 1;
    data.cultivatableLayer = {{GameConstants::CultivatableGrassGid,
                               GameConstants::CultivatableGrassGid}};
    data.cultivatedLayer = {{GameConstants::CultivatedTileGid,
                             GameConstants::CultivatedTileGid}};
    data.cropLayer = {{0, 0}};

    WorldState state;
    state.initialize(data);
    const QPoint cropTile(0, 0);
    const QPoint emptyTile(1, 0);
    QVERIFY(state.plantSeed(cropTile, CropType::Wheat, 0));

    const DayAdvanceResult dryDay = state.advanceDay(0, []() { return 99; });
    QCOMPARE(dryDay.cropsAdvanced, 0);
    QCOMPARE(dryDay.cropsStalled, 1);
    QCOMPARE(state.cropGidAt(cropTile), GameConstants::CropFirstStageGid);

    QVERIFY(state.waterTile(cropTile));
    QVERIFY(state.isWatered(cropTile));
    QCOMPARE(state.cultivatedLayer().at(0).at(0),
             GameConstants::CultivatedTileGid);
    const DayAdvanceResult wateredDay = state.advanceDay(1, []() { return 0; });
    QCOMPARE(wateredDay.cropsAdvanced, 1);
    QCOMPARE(wateredDay.soilRecovered, 1);
    QVERIFY(!state.isWatered(cropTile));
    QCOMPARE(state.cultivatedLayer().at(0).at(0),
             GameConstants::CultivatedTileGid);
    QCOMPARE(state.cultivatedLayer().at(0).at(1), 0);
    QVERIFY(state.isTileCultivatable(emptyTile));
}

void CoreTests::inventoryEnforcesWholeHarvestCapacity() {
    Inventory inventory(4);
    QVERIFY(inventory.add(CropType::Wheat, 2));
    QVERIFY(!inventory.add(CropType::Potato, 3));
    QCOMPARE(inventory.usedCapacity(), 2);
    QCOMPARE(inventory.count(CropType::Potato), 0);
    QVERIFY(inventory.add(CropType::Potato, 2));
    QCOMPARE(inventory.remainingCapacity(), 0);

    MapData data;
    data.mapWidth = 1;
    data.mapHeight = 1;
    data.cultivatedLayer = {{GameConstants::CultivatedTileGid}};
    WorldState state;
    state.initialize(data);
    QVERIFY(state.plantSeed(QPoint(0, 0), CropType::Potato, 0));
    for (int day = 0; day < CropCatalog::definition(CropType::Potato).growthDays;
         ++day) {
        QVERIFY(state.waterTile(QPoint(0, 0)));
        state.advanceDay(day, []() { return 99; });
    }
    const HarvestResult preview = state.previewHarvest(QPoint(0, 0));
    QCOMPARE(preview.quantity, 3);
    QVERIFY(!inventory.add(preview.type, preview.quantity));
    QCOMPARE(state.cropGidAt(QPoint(0, 0)), GameConstants::CropMatureStageGid);
}

void CoreTests::economyRequiresMarketAndSellsAtomically() {
    EconomyService economy(12);
    QVERIFY(economy.collectHarvest(CropType::Wheat, 2));
    QVERIFY(economy.collectHarvest(CropType::Potato, 3));
    const QRect market(5, 5, 2, 2);

    const SaleResult farSale = economy.sellAll(QPoint(0, 0), market);
    QVERIFY(!farSale.success);
    QCOMPARE(economy.coins(), 0);
    QCOMPARE(economy.inventory().usedCapacity(), 5);

    const SaleResult sale = economy.sellAll(QPoint(5, 5), market);
    QVERIFY(sale.success);
    QCOMPARE(sale.itemsSold, 5);
    QCOMPARE(sale.coinsEarned, 17);
    QCOMPARE(economy.coins(), 17);
    QCOMPARE(economy.inventory().usedCapacity(), 0);

    QVERIFY(!economy.sellAll(QPoint(5, 5), market).success);
    QCOMPARE(economy.coins(), 17);
}

void CoreTests::seedsAreConsumedAndPurchasedAtomically() {
    EconomyService economy(12);
    QCOMPARE(economy.seedCount(CropType::Wheat),
             GameConstants::InitialWheatSeeds);
    QCOMPARE(economy.seedCount(CropType::Potato),
             GameConstants::InitialPotatoSeeds);

    QVERIFY(!economy.plantWithSeed(CropType::Wheat,
                                   []() { return false; }));
    QCOMPARE(economy.seedCount(CropType::Wheat),
             GameConstants::InitialWheatSeeds);
    QVERIFY(economy.plantWithSeed(CropType::Wheat,
                                  []() { return true; }));
    QCOMPARE(economy.seedCount(CropType::Wheat),
             GameConstants::InitialWheatSeeds - 1);

    const SeedPurchaseResult noMoney = economy.buySeeds(CropType::Potato, 1);
    QVERIFY(!noMoney.success);
    QCOMPARE(noMoney.failure, SeedPurchaseFailure::InsufficientCoins);
    QCOMPARE(economy.seedCount(CropType::Potato),
             GameConstants::InitialPotatoSeeds);

    QVERIFY(economy.collectHarvest(CropType::Wheat, 2));
    const QRect market(1, 1, 1, 1);
    QCOMPARE(economy.sellAll(QPoint(1, 1), market).coinsEarned, 8);
    const SeedPurchaseResult purchase = economy.buySeeds(CropType::Potato, 1);
    QVERIFY(purchase.success);
    QCOMPARE(purchase.coinsSpent, 5);
    QCOMPARE(economy.coins(), 3);
    QCOMPARE(economy.seedCount(CropType::Potato),
             GameConstants::InitialPotatoSeeds + 1);

    const int coinsBeforeInvalid = economy.coins();
    QVERIFY(!economy.buySeeds(CropType::Potato, 0).success);
    QCOMPARE(economy.coins(), coinsBeforeInvalid);
}

void CoreTests::saveManagerRoundTripsGameStateAndKeepsBackup() {
    MapData mapData;
    mapData.mapWidth = 2;
    mapData.mapHeight = 1;
    mapData.cultivatableLayer = {{GameConstants::CultivatableGrassGid,
                                  GameConstants::CultivatableGrassGid}};
    mapData.cultivatedLayer = {{0, 0}};
    mapData.cropLayer = {{0, 0}};

    WorldState world;
    world.initialize(mapData);
    QVERIFY(world.cultivate(QPoint(0, 0)));
    QVERIFY(world.plantSeed(QPoint(0, 0), CropType::Wheat, 2));
    QVERIFY(world.waterTile(QPoint(0, 0)));

    EconomyService economy(12);
    QVERIFY(economy.plantWithSeed(CropType::Wheat,
                                  []() { return true; }));

    GameSaveData original;
    original.gameMinutes = 2 * 24 * 60 + 9 * 60 + 15;
    original.lastProcessedDay = 2;
    original.playerPosition = QPoint(250, 170);
    original.economy = economy.state();
    original.world = world.snapshot();

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("savegame.json"));
    QString error;
    QVERIFY2(SaveManager::save(path, original, &error), qPrintable(error));

    GameSaveData loaded;
    QVERIFY2(SaveManager::load(path, &loaded, &error), qPrintable(error));
    QCOMPARE(loaded.gameMinutes, original.gameMinutes);
    QCOMPARE(loaded.lastProcessedDay, 2);
    QCOMPARE(loaded.playerPosition, QPoint(250, 170));
    QCOMPARE(loaded.economy.coins, original.economy.coins);
    QCOMPARE(loaded.economy.seedCounts.value(static_cast<int>(CropType::Wheat)),
             GameConstants::InitialWheatSeeds - 1);
    QCOMPARE(loaded.world.crops.size(), 1);
    QCOMPARE(loaded.world.crops.first().type, CropType::Wheat);
    QVERIFY(loaded.world.wateredTiles.contains(QPoint(0, 0)));

    EconomyService restoredEconomy;
    WorldState restoredWorld;
    restoredWorld.initialize(mapData);
    QVERIFY(restoredEconomy.restore(loaded.economy));
    QVERIFY(restoredWorld.restore(loaded.world));
    QCOMPARE(restoredEconomy.seedCount(CropType::Wheat),
             GameConstants::InitialWheatSeeds - 1);
    QVERIFY(restoredWorld.isWatered(QPoint(0, 0)));
    QCOMPARE(restoredWorld.cropGidAt(QPoint(0, 0)),
             GameConstants::CropFirstStageGid);

    GameSaveData updated = original;
    updated.gameMinutes += 60;
    QVERIFY2(SaveManager::save(path, updated, &error), qPrintable(error));
    QVERIFY(QFileInfo::exists(path + QStringLiteral(".bak")));
    GameSaveData backup;
    QVERIFY2(SaveManager::load(path + QStringLiteral(".bak"), &backup, &error),
             qPrintable(error));
    QCOMPARE(backup.gameMinutes, original.gameMinutes);
}

QTEST_APPLESS_MAIN(CoreTests)

#include "core_tests.moc"
