#include <QtTest>

#include "player/collisiondetector.h"
#include "farm/cropdefinition.h"
#include "economy/economyservice.h"
#include "core/gameclock.h"
#include "core/gameconstants.h"
#include "map/mapdata.h"
#include "storage/savemanager.h"
#include "farm/worldstate.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "farm/seasoncalendar.h"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <limits>

class CoreTests : public QObject {
    Q_OBJECT

private slots:
    void wholeTreesChopPersistAndGiveWood();
    void upgradedAxeKeepsTreeYields();
    void woodFullBagAndInvalidActionsAreAtomic();
    void gameClockConvertsElapsedTime();
    void seasonCalendarUsesTheClockDate();
    void cropCatalogHasIndependentStagesAndIcons();
    void seasonalPlantingPreservesSeedsAndExistingCrops();
    void offSeasonProgressSurvivesCatchupAndReturningSeason();
    void allCropTypesCompleteTheEconomicLoop();
    void catchingUpAQuietFarmMatchesDailySettlement();
    void fertilizerWaitsForWaterAndAppliesOnce();
    void fertilizerRejectsInvalidTargetsAndSnapshots();
    void fertilizedCropsSaveAndRejectMalformedFlags();

    void gameClockOnlySleepsAtNight();
    void gameClockRunsThroughMidnight();
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

void CoreTests::wholeTreesChopPersistAndGiveWood() {
    MapData map; QString error;
    map.mapWidth=72;map.mapHeight=40;
    map.treeLayer=QVector<QVector<int>>(40,QVector<int>(72));
    map.collisionLayer=map.treeLayer;
    for(int y=0;y<2;++y) for(int x=0;x<2;++x) {
        map.treeLayer[2+y][43+x]=338+y*12+x;
        map.collisionLayer[2+y][43+x]=1;
    }
    WorldState world;world.initialize(map);
    EconomyService economy; QVERIFY(economy.ensureAxe());
    const auto axes=economy.bagSlots(); QVERIFY(economy.ensureAxe()); QCOMPARE(economy.bagSlots(),axes);
    const QPoint trunk(44,3);
    QVERIFY(!world.previewTreeChop(QPoint(43,2)).success); // canopy is not a trunk
    for(int i=0;i<4;++i) {
        const auto preview=world.previewTreeChop(trunk);QVERIFY(preview.success);
        QCOMPARE(preview.wood,i==3?5:0);
        QVERIFY(economy.collectWoodWithAction(preview.wood,[&]{return world.commitTreeChop(preview);}));
        QVERIFY(!world.commitTreeChop(preview)); // stale replay cannot damage it twice
    }
    QCOMPARE(world.treeGidAt({43,2}),0);
    QCOMPARE(world.treeGidAt(trunk),409);
    QCOMPARE(world.collisionLayer()[3][44],1);
    QCOMPARE(world.collisionLayer()[2][43],0);
    QCOMPARE(economy.state().woodCount,5);
    GameSaveData save;save.gameMinutes=360;save.world=world.snapshot();save.economy=economy.state();
    save.playerPosition={400,192};QTemporaryDir dir;QVERIFY(dir.isValid());
    const QString path=dir.filePath("tree.json");QVERIFY(SaveManager::save(path,save,&error));
    GameSaveData loaded;QVERIFY2(SaveManager::load(path,&loaded,&error),qPrintable(error));
    WorldState restored;restored.initialize(map);QVERIFY(restored.restore(loaded.world));
    QCOMPARE(restored.treeGidAt(trunk),409);
    EconomyService assets;QVERIFY(assets.restore(loaded.economy));QCOMPARE(assets.state().woodCount,5);
    for(int i=0;i<2;++i) {
        const auto preview=restored.previewTreeChop(trunk);
        QVERIFY(assets.collectWoodWithAction(preview.wood,[&]{return restored.commitTreeChop(preview);}));
    }
    QCOMPARE(restored.treeGidAt(trunk),0);QCOMPARE(restored.collisionLayer()[3][44],0);
    QCOMPARE(assets.state().woodCount,7);QVERIFY(!restored.previewTreeChop(trunk).success);
    QVERIFY(assets.transfer(StorageItemKind::Wood,CropType::None,StorageDirection::Deposit,3).success);
    QCOMPARE(assets.state().woodCount,4);QCOMPARE(assets.storage().woodCount,3);
    save.world=restored.snapshot();save.economy=assets.state();
    QVERIFY(SaveManager::save(path,save,&error));
    QVERIFY(SaveManager::load(path,&loaded,&error));
    WorldState cleared;cleared.initialize(map);QVERIFY(cleared.restore(loaded.world));
    QCOMPARE(cleared.treeGidAt(trunk),0);QCOMPARE(loaded.economy.storage.woodCount,3);
    const QByteArray validFile=[&]{QFile f(path);f.open(QIODevice::ReadOnly);return f.readAll();}();
    save.world.trees[0].anchor={1000,1000};QVERIFY(!SaveManager::save(path,save,&error));
    {QFile f(path);QVERIFY(f.open(QIODevice::ReadOnly));QCOMPARE(f.readAll(),validFile);}
    const auto current=world.snapshot();
    auto bad=current;bad.trees[0].health=-1;QVERIFY(!world.restore(bad));
    QCOMPARE(world.treeGidAt(trunk),409);
}

void CoreTests::upgradedAxeKeepsTreeYields() {
    MapData map;
    map.mapWidth = 1;
    map.mapHeight = 2;
    map.treeLayer = {{335}, {347}};
    const QPoint trunk(0, 1);
    WorldState world;
    world.initialize(map);
    const auto fresh = world.snapshot();
    QVERIFY(!world.previewTreeChop(trunk, 0).success);
    QVERIFY(!world.previewTreeChop({0, 0}, 50).success);
    for (int oldHits : {0, 1, 3}) {
        QVERIFY(world.restore(fresh));
        for (int hit = 0; hit < oldHits; ++hit) {
            const auto normal = world.previewTreeChop(trunk);
            QCOMPARE(normal.wood, 0);
            QVERIFY(world.commitTreeChop(normal));
        }
        QVERIFY(world.restore(world.snapshot())); // A tree from an ordinary axe save.
        const int standingHits = oldHits == 3 ? 1 : 2;
        for (int hit = 1; hit <= standingHits; ++hit) {
            const auto upgraded = world.previewTreeChop(trunk, 50);
            QCOMPARE(upgraded.wood, hit == standingHits ? 5 : 0);
            QCOMPARE(upgraded.felled, hit == standingHits);
            QVERIFY(world.commitTreeChop(upgraded));
            QVERIFY(!world.commitTreeChop(upgraded));
        }
        const auto stump = world.previewTreeChop(trunk, 50);
        QCOMPARE(stump.wood, 2);
        QVERIFY(world.commitTreeChop(stump));
        QVERIFY(!world.commitTreeChop(stump));
        QVERIFY(!world.previewTreeChop(trunk, 50).success);
    }
}

void CoreTests::woodFullBagAndInvalidActionsAreAtomic() {
    EconomyService economy;auto state=economy.state();state.seedCounts[1]=100;QVERIFY(economy.restore(state));
    for(int i=0;i<21;++i) if(!economy.bagSlots()[i].item) {
        StackMove move;move.source=2;move.target=i;move.quantity=1;
        move.expected=economy.bagSlots()[2];move.revision=economy.revision();QVERIFY(economy.moveStack(move));
    }
    bool called=false;const auto before=economy.state();
    QVERIFY(!economy.collectWoodWithAction(5,[&]{called=true;return true;}));
    QVERIFY(!called);QCOMPARE(economy.bagSlots(),before.bagSlots);QCOMPARE(economy.state().woodCount,0);
    QVERIFY(!economy.collectWoodWithAction(-1,[]{return true;}));
    EconomyService free;
    QVERIFY(!free.collectWoodWithAction(5,[]{return false;}));QCOMPARE(free.state().woodCount,0);
    bool reentered=true;
    QVERIFY(free.collectWoodWithAction(5,[&]{reentered=free.collectWoodWithAction(1,[]{return true;});return true;}));
    QVERIFY(!reentered);QCOMPARE(free.state().woodCount,5);QVERIFY(EconomyService::layoutMatches(free.state()));
}

void CoreTests::gameClockConvertsElapsedTime() {
    constexpr int speed = 1000;
    QCOMPARE(GameClock::minuteFromElapsed(90 * 1000, speed), 30);
    QCOMPARE(GameClock::hourFromElapsed(90 * 1000, speed), 1);
    QCOMPARE(GameClock::dayFromElapsed(24LL * 60 * 1000, speed), 1);

    GameClock clock(1000000);
    clock.start();
    clock.setElapsedGameMinutes(20 * 60);
    QVERIFY(clock.canSleep(20));
    QVERIFY(clock.sleepToNextMorning(6, 20));
    QCOMPARE(clock.day(), 1);
    QCOMPARE(clock.hour(), 6);
    clock.setElapsedGameMinutes(2 * 24 * 60 + 13 * 60 + 25);
    QCOMPARE(clock.day(), 2);
    QCOMPARE(clock.hour(), 13);
    QCOMPARE(clock.minute(), 25);
}

void CoreTests::gameClockOnlySleepsAtNight() {
    GameClock clock(1000000);
    clock.setElapsedGameMinutes(6 * 60);
    clock.start();
    QVERIFY(!clock.canSleep(20));
    QVERIFY(!clock.sleepToNextMorning(6, 20));
    QCOMPARE(clock.day(), 0);
    QCOMPARE(clock.hour(), 6);

    clock.setElapsedGameMinutes(19 * 60 + 59);
    QVERIFY(!clock.canSleep(20));
    QVERIFY(!clock.sleepToNextMorning(6, 20));
    QCOMPARE(clock.elapsedGameMinutes(), 19LL * 60 + 59);
    clock.setElapsedGameMinutes(20 * 60);
    QVERIFY(clock.canSleep(20));
    QVERIFY(clock.sleepToNextMorning(6, 20));
    QCOMPARE(clock.day(), 1);
    QCOMPARE(clock.hour(), 6);
    QVERIFY(!clock.sleepToNextMorning(6, 20));
    QCOMPARE(clock.day(), 1);

    for (const int minuteOfDay : {0, 1, 119, 120, 5 * 60 + 59, 20 * 60, 23 * 60 + 59}) {
        const int currentDay = 6;
        clock.setElapsedGameMinutes(currentDay * 24 * 60 + minuteOfDay);
        QVERIFY(clock.canSleep());
        QVERIFY(clock.sleepToNextMorning());
        QCOMPARE(clock.day(), currentDay + (minuteOfDay >= 20 * 60 ? 1 : 0));
        QCOMPARE(clock.hour(), 6);
        QCOMPARE(clock.minute(), 0);
        QVERIFY(!clock.canSleep());
    }
    QVERIFY(!GameClock::sleepTargetMinute(-1));
    QVERIFY(!GameClock::sleepTargetMinute(0, 24));
    QVERIFY(!GameClock::sleepTargetMinute(6 * 60));
    QVERIFY(!GameClock::sleepTargetMinute(19 * 60 + 59));
    QCOMPARE(GameClock::sleepTargetMinute(120).value(), 360LL);
    QCOMPARE(GameClock::sleepTargetMinute(20 * 60).value(), 1800LL);
    const qint64 lastDay = qint64(std::numeric_limits<int>::max()) * 1440;
    clock.setElapsedGameMinutes(lastDay + 20 * 60);
    QVERIFY(!clock.sleepToNextMorning());
    QCOMPARE(clock.elapsedGameMinutes(), lastDay + 20 * 60);
}

void CoreTests::gameClockRunsThroughMidnight() {
    GameClock clock(100);
    clock.setElapsedGameMinutes(24 * 60 - 1);
    QTRY_VERIFY_WITH_TIMEOUT(clock.elapsedGameMinutes() >= 24 * 60, 2000);
    QCOMPARE(clock.day(), 1);
    QCOMPARE(clock.hour(), 0);
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
    data.cultivatableLayer = {{GameConstants::CultivatableGrassGid, GameConstants::CultivatableGrassGid}, {0, 0}};
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
    QVERIFY(!state.cultivate(tree));
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
    QCOMPARE(wheat.sellPrice, 5);
    QCOMPARE(potato.growthDays, 5);
    QCOMPARE(potato.harvestYield, 3);
    QCOMPARE(potato.seedPrice, 4);
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
    QCOMPARE(state.cropGidAt(QPoint(0, 0)), CropCatalog::definition(CropType::Potato).stageGids.last());
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
    QCOMPARE(sale.coinsEarned, 19);
    QCOMPARE(economy.coins(), 19);
    QCOMPARE(economy.inventory().usedCapacity(), 0);

    QVERIFY(!economy.sellAll(QPoint(5, 5), market).success);
    QCOMPARE(economy.coins(), 19);
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
    QCOMPARE(economy.sellAll(QPoint(1, 1), market).coinsEarned, 10);
    const SeedPurchaseResult purchase = economy.buySeeds(CropType::Potato, 1);
    QVERIFY(purchase.success);
    QCOMPARE(purchase.coinsSpent, 4);
    QCOMPARE(economy.coins(), 6);
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
    world.advanceDay(0);
    world.advanceDay(1);
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


void CoreTests::seasonCalendarUsesTheClockDate() {
    struct Boundary { qint64 day; qint64 year; Season season; int date; };
    const Boundary boundaries[] = {
        {0,1,Season::Spring,1}, {6,1,Season::Spring,7},
        {7,1,Season::Summer,1}, {13,1,Season::Summer,7},
        {14,1,Season::Autumn,1}, {20,1,Season::Autumn,7},
        {21,1,Season::Winter,1}, {27,1,Season::Winter,7},
        {28,2,Season::Spring,1}, {280021,10001,Season::Winter,1}
    };
    for (const auto &b : boundaries) {
        const SeasonDate date = SeasonCalendar::dateForDay(b.day);
        QCOMPARE(date.year,b.year); QCOMPARE(date.season,b.season);
        QCOMPARE(date.dayOfSeason,b.date);
        const SeasonDate fromMinutes = SeasonCalendar::dateForMinutes(b.day*1440+1439);
        QCOMPARE(fromMinutes.year,b.year); QCOMPARE(fromMinutes.season,b.season);
        QCOMPARE(fromMinutes.dayOfSeason,b.date);
    }
    QCOMPARE(SeasonCalendar::dateForDay(-1).dayOfSeason,1);
    QCOMPARE(SeasonCalendar::dateForMinutes(7*1440-1).season,Season::Spring);
    QCOMPARE(SeasonCalendar::dateForMinutes(7*1440).season,Season::Summer);
    QVERIFY(SeasonCalendar::seasonId(static_cast<Season>(99)).isEmpty());
}

void CoreTests::cropCatalogHasIndependentStagesAndIcons() {
    QCOMPARE(CropCatalog::cropTypes().size(),4);
    QSet<QString> ids;
    QSet<int> stageIds;
    for (CropType type : CropCatalog::cropTypes()) {
        const auto &crop = CropCatalog::definition(type);
        QVERIFY(!ids.contains(crop.id)); ids.insert(crop.id);
        QCOMPARE(CropCatalog::fromId(crop.id),type);
        QVERIFY(!crop.localizedName.isEmpty());
        QVERIFY(crop.growthDays>0 && crop.harvestYield>0);
        QVERIFY(crop.sellPrice*crop.harvestYield>crop.seedPrice);
        QVERIFY(!crop.seedIcon.resourcePath.isEmpty());
        QCOMPARE(crop.seedIcon.sourceRect.size(),QSize(16,16));
        QCOMPARE(crop.harvestIcon.sourceRect.size(),QSize(16,16));
        QVERIFY(crop.stageGids.size()>=3);
        for (int gid : crop.stageGids) {
            QVERIFY(!stageIds.contains(gid)); stageIds.insert(gid);
        }
        QCOMPARE(CropCatalog::stageGid(type,0),crop.stageGids.first());
        QCOMPARE(CropCatalog::stageGid(type,crop.growthDays),crop.stageGids.last());
        QCOMPARE(CropCatalog::stageGid(type,crop.growthDays+100),crop.stageGids.last());
    }
    for (Season season : {Season::Spring,Season::Summer,Season::Autumn,Season::Winter})
        QVERIFY(CropCatalog::canPlant(CropType::Wheat,season));
    QVERIFY(!CropCatalog::canPlant(CropType::None,Season::Spring));
    QVERIFY(!CropCatalog::canPlant(CropType::Wheat,static_cast<Season>(-1)));
    QVERIFY(!CropCatalog::canPlantOnDay(CropType::Wheat,-1));
    QCOMPARE(CropCatalog::fromId(QStringLiteral("missing")),CropType::None);
}

void CoreTests::seasonalPlantingPreservesSeedsAndExistingCrops() {
    MapData data; data.mapWidth=2; data.mapHeight=1;
    data.cultivatedLayer={{135,135}};
    WorldState world; world.initialize(data);
    EconomyService economy;
    EconomyState assets=economy.state();
    assets.seedCounts.insert(static_cast<int>(CropType::Carrot),2);
    QVERIFY(economy.restore(assets));
    // The last spring evening permits planting; the same crop survives summer.
    QVERIFY(economy.plantWithSeed(CropType::Carrot,[&](){return world.plantSeed({0,0},CropType::Carrot,6);}));
    QCOMPARE(economy.seedCount(CropType::Carrot),1);
    QVERIFY(world.waterTile({0,0}));world.advanceDay(6,[](){return 99;});
    const int springGid=world.cropGidAt({0,0});
    QVERIFY(!economy.plantWithSeed(CropType::Carrot,[&](){return world.plantSeed({1,0},CropType::Carrot,7);}));
    QCOMPARE(economy.seedCount(CropType::Carrot),1);
    QCOMPARE(world.cropGidAt({1,0}),0);
    QCOMPARE(world.snapshot().crops.first().growthDays, 1);
    QVERIFY(world.fertilize({0, 0}).success);
    QVERIFY(world.waterTile({0, 0}));
    const auto firstSummerDay = world.advanceDay(7, []() { return 99; });
    QCOMPARE(firstSummerDay.cropsAdvanced, 1);
    QCOMPARE(firstSummerDay.cropsStalled, 0);
    QCOMPARE(firstSummerDay.fertilizerBonusesUsed, 0);
    QCOMPARE(world.cropGidAt({0, 0}), springGid);
    const auto halfway = world.snapshot();
    QVERIFY(halfway.crops.first().offSeasonHalfGrowth);
    WorldState restored;
    restored.initialize(data);
    QVERIFY(restored.restore(halfway));
    QVERIFY(restored.hasPendingFertilizerBonus({0, 0}));
    QCOMPARE(restored.advanceDay(8, []() { return 99; }).cropsStalled, 1);
    QVERIFY(restored.snapshot().crops.first().offSeasonHalfGrowth);
    QVERIFY(restored.waterTile({0, 0}));
    QCOMPARE(restored.advanceDay(9, []() { return 99; }).fertilizerBonusesUsed, 1);
    QCOMPARE(restored.snapshot().crops.first().growthDays, 3);
    QVERIFY(!restored.snapshot().crops.first().offSeasonHalfGrowth);
    for (int day : {10, 11}) {
        QVERIFY(restored.waterTile({0, 0}));
        QCOMPARE(restored.advanceDay(day, []() { return 99; }).fertilizerBonusesUsed, 0);
    }
    QVERIFY(restored.previewHarvest({0, 0}).isValid());
    auto invalid = restored.snapshot();
    invalid.crops.first().offSeasonHalfGrowth = true;
    QVERIFY(!restored.restore(invalid)); // Mature plants cannot have a growth remainder.
    QVERIFY(restored.previewHarvest({0, 0}).isValid());
    QVERIFY(!restored.plantSeed({1, 0}, CropType::Wheat, -1));
    QVERIFY(!restored.plantSeed({1, 0}, static_cast<CropType>(99), 12));
    restored.advanceDay(11, []() { return 0; });
    QCOMPARE(restored.snapshot().lastCompletedDay, 11);
}

void CoreTests::offSeasonProgressSurvivesCatchupAndReturningSeason() {
    MapData map;
    map.mapWidth = 1;
    map.mapHeight = 1;
    map.cultivatedLayer = {{135}};
    WorldState world;
    world.initialize(map);
    const QPoint field(0, 0);
    QVERIFY(world.plantSeed(field, CropType::Carrot, 6));
    world.advanceDay(6);
    QVERIFY(world.waterTile(field));
    const auto summer = world.advanceThroughDay(13); // One watered summer day, six dry.
    QCOMPARE(summer.cropsAdvanced, 1);
    QCOMPARE(summer.cropsStalled, 6);
    QCOMPARE(world.snapshot().crops.first().growthDays, 0);
    QVERIFY(world.snapshot().crops.first().offSeasonHalfGrowth);

    QVERIFY(world.waterTile(field));
    world.advanceThroughDay(20); // First autumn watering grants a full day.
    QCOMPARE(world.snapshot().crops.first().growthDays, 1);
    QVERIFY(world.snapshot().crops.first().offSeasonHalfGrowth);
    QVERIFY(world.waterTile(field));
    world.advanceDay(21); // First winter watering completes the saved half day.
    QCOMPARE(world.snapshot().crops.first().growthDays, 2);
    QVERIFY(!world.snapshot().crops.first().offSeasonHalfGrowth);

    world.initialize(map);
    QVERIFY(world.plantSeed(field, CropType::Wheat, 6));
    for (int day : {6, 7, 8}) {
        QVERIFY(world.waterTile(field));
        world.advanceDay(day);
    }
    QVERIFY(world.previewHarvest(field).isValid());
}

void CoreTests::allCropTypesCompleteTheEconomicLoop() {
    int totalSeedCost = 0;
    int totalSales = 0;
    for (CropType type : CropCatalog::cropTypes()) {
        const auto &crop = CropCatalog::definition(type);
        int firstDay = 0;
        while (!CropCatalog::canPlantOnDay(type, firstDay))
            ++firstDay;
        MapData map;
        map.mapWidth = 1;
        map.mapHeight = 1;
        map.cultivatedLayer = {{GameConstants::CultivatedTileGid}};
        WorldState world;
        world.initialize(map);
        EconomyService economy;
        auto assets = economy.state();
        assets.coins = 100;
        QVERIFY(economy.restore(assets));

        const int previousSeeds = economy.seedCount(type);
        const auto purchase = economy.buySeeds(type, 1);
        QVERIFY(purchase.success);
        totalSeedCost += purchase.coinsSpent;
        QVERIFY(economy.plantWithSeed(type, [&]() {
            return world.plantSeed({0, 0}, type, firstDay);
        }));
        QCOMPARE(economy.seedCount(type), previousSeeds);
        for (int offset = 0; offset < crop.growthDays; ++offset) {
            QVERIFY(world.waterTile({0, 0}));
            QCOMPARE(world.advanceDay(firstDay + offset, []() { return 99; }).cropsAdvanced, 1);
        }

        const auto harvest = world.previewHarvest({0, 0});
        QCOMPARE(harvest.type, type);
        QCOMPARE(harvest.quantity, crop.harvestYield);
        QVERIFY(economy.collectHarvest(type, harvest.quantity));
        QVERIFY(world.harvest({0, 0}).isValid());
        const QRect market(2, 2, 1, 1);
        const auto sale = economy.sellAll({2, 2}, market);
        QVERIFY(sale.success);
        totalSales += sale.coinsEarned;
        QCOMPARE(economy.coins(), 100 - purchase.coinsSpent + sale.coinsEarned);
        QVERIFY(!world.harvest({0, 0}).isValid());
        QVERIFY(!economy.sellAll({2, 2}, market).success);
    }
    // One ordinary harvest per variety: pay 11 coins for seeds and receive 28.
    QCOMPARE(totalSeedCost, 11);
    QCOMPARE(totalSales, 28);
}

void CoreTests::catchingUpAQuietFarmMatchesDailySettlement() {
    MapData map;
    map.mapWidth = 3;
    map.mapHeight = 1;
    map.cultivatedLayer = {{135, 135, 135}};
    WorldState daily;
    daily.initialize(map);
    QVERIFY(daily.plantSeed({0, 0}, CropType::Wheat, 0));
    QVERIFY(daily.plantSeed({1, 0}, CropType::Potato, 0));
    QVERIFY(daily.plantSeed({2, 0}, CropType::Beet, 14));
    QVERIFY(daily.fertilize({0, 0}).success);
    QVERIFY(daily.waterTile({0, 0}));
    QVERIFY(daily.waterTile({1, 0}));
    WorldState catchup;
    catchup.initialize(map);
    QVERIFY(catchup.restore(daily.snapshot()));

    qint64 expectedStalls = 0;
    for (int day = 0; day <= 30; ++day)
        expectedStalls += daily.advanceDay(day).cropsStalled;
    const auto result = catchup.advanceThroughDay(30);
    QCOMPARE(result.cropsAdvanced, 2);
    QCOMPARE(result.fertilizerBonusesUsed, 1);
    QCOMPARE(result.cropsStalled, expectedStalls);
    QCOMPARE(catchup.cropLayer(), daily.cropLayer());
    QCOMPARE(catchup.snapshot().wateredTiles, daily.snapshot().wateredTiles);
    QVERIFY(!catchup.hasPendingFertilizerBonus({0, 0}));

    constexpr int distantDay = 1000000000;
    const auto distant = catchup.advanceThroughDay(distantDay);
    QCOMPARE(distant.cropsStalled, qint64(3) * (distantDay - 30));
    QCOMPARE(distant.cropsAdvanced, 0);
    QCOMPARE(catchup.snapshot().lastCompletedDay, distantDay);
    QCOMPARE(catchup.advanceThroughDay(distantDay).cropsStalled, 0);
}

void CoreTests::fertilizerWaitsForWaterAndAppliesOnce() {
    MapData data;data.mapWidth=1;data.mapHeight=1;data.cultivatedLayer={{135}};
    WorldState world;world.initialize(data);
    QVERIFY(world.plantSeed({0,0},CropType::Wheat,0));
    QVERIFY(world.fertilize({0,0}).success);
    QVERIFY(world.isFertilized({0,0}));QVERIFY(world.hasPendingFertilizerBonus({0,0}));
    QCOMPARE(world.fertilize({0,0}).failure,FertilizeFailure::AlreadyApplied);
    const auto dry=world.advanceDay(0,[](){return 99;});
    QCOMPARE(dry.cropsAdvanced,0);QCOMPARE(dry.fertilizerBonusesUsed,0);
    QVERIFY(world.hasPendingFertilizerBonus({0,0}));
    QVERIFY(world.waterTile({0,0}));
    auto wet=world.advanceDay(1,[](){return 99;});
    QCOMPARE(wet.cropsAdvanced,1);QCOMPARE(wet.fertilizerBonusesUsed,1);
    QCOMPARE(world.snapshot().crops.first().growthDays,2);
    QVERIFY(!world.hasPendingFertilizerBonus({0,0}));
    QCOMPARE(world.advanceDay(1,[](){return 99;}).cropsAdvanced,0);
    QCOMPARE(world.snapshot().crops.first().growthDays,2);
    QVERIFY(world.waterTile({0,0}));world.advanceDay(2,[](){return 99;});
    QVERIFY(world.previewHarvest({0,0}).isValid());
    QVERIFY(world.harvest({0,0}).isValid());
    QVERIFY(!world.isFertilized({0,0}));
    QVERIFY(world.plantSeed({0,0},CropType::Wheat,3));
    QVERIFY(world.fertilize({0,0}).success);
}

void CoreTests::fertilizerRejectsInvalidTargetsAndSnapshots() {
    MapData data;data.mapWidth=2;data.mapHeight=1;data.cultivatedLayer={{135,135}};
    WorldState world;world.initialize(data);
    QCOMPARE(world.fertilize({-1,0}).failure,FertilizeFailure::InvalidTile);
    QCOMPARE(world.fertilize({0,0}).failure,FertilizeFailure::NoCrop);
    QVERIFY(world.plantSeed({0,0},CropType::Wheat,0));
    for(int day=0;day<3;++day) {QVERIFY(world.waterTile({0,0}));world.advanceDay(day,[](){return 99;});}
    QCOMPARE(world.fertilize({0,0}).failure,FertilizeFailure::AlreadyMature);
    QVERIFY(!world.isFertilized({0,0}));
    QVERIFY(world.plantSeed({1,0},CropType::Wheat,3));
    QVERIFY(world.fertilize({1,0}).success);
    const auto valid=world.snapshot();auto bad=valid;
    for(auto &crop:bad.crops)if(crop.tilePos==QPoint(1,0))crop.fertilizerApplied=false;
    QVERIFY(!world.restore(bad));
    auto matureBonus=valid;
    for(auto &crop:matureBonus.crops)if(crop.tilePos==QPoint(0,0)) {
        crop.fertilizerApplied=true;crop.fertilizerBonusPending=true;
    }
    QVERIFY(!world.restore(matureBonus));
    QVERIFY(world.hasPendingFertilizerBonus({1,0}));
    QCOMPARE(world.snapshot().lastCompletedDay,valid.lastCompletedDay);
    WorldState fresh;fresh.initialize(data);QVERIFY(fresh.restore(valid));
    QVERIFY(fresh.hasPendingFertilizerBonus({1,0}));
}

void CoreTests::fertilizedCropsSaveAndRejectMalformedFlags() {
    MapData map;map.mapWidth=1;map.mapHeight=1;map.cultivatedLayer={{135}};
    WorldState world;world.initialize(map);
    for (int day=0;day<14;++day) world.advanceDay(day,[](){return 99;});
    QVERIFY(world.plantSeed({0,0},CropType::Beet,14));
    QVERIFY(world.fertilize({0,0}).success);
    GameSaveData state;state.gameMinutes=14*1440+360;state.lastProcessedDay=14;
    EconomyService economy;auto assets=economy.state();assets.seedCounts.insert(static_cast<int>(CropType::Beet),2);
    QVERIFY(economy.restore(assets));state.economy=economy.state();state.world=world.snapshot();
    QTemporaryDir dir;const auto path=dir.filePath(QStringLiteral("fertilized.json"));QString error;
    QVERIFY2(SaveManager::save(path,state,&error),qPrintable(error));
    GameSaveData loaded;QVERIFY2(SaveManager::load(path,&loaded,&error),qPrintable(error));
    QCOMPARE(loaded.world.crops.first().type,CropType::Beet);
    QVERIFY(loaded.world.crops.first().fertilizerApplied);
    QVERIFY(loaded.world.crops.first().fertilizerBonusPending);
    WorldState restored;restored.initialize(map);QVERIFY(restored.restore(loaded.world));
    QVERIFY(restored.waterTile({0,0}));
    QCOMPARE(restored.advanceDay(14,[](){return 99;}).fertilizerBonusesUsed,1);
    QCOMPARE(restored.advanceDay(14,[](){return 99;}).fertilizerBonusesUsed,0);
    QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));
    auto document=QJsonDocument::fromJson(file.readAll());file.close();
    auto root=document.object();auto w=root.value(QStringLiteral("world")).toObject();
    auto crops=w.value(QStringLiteral("crops")).toArray();auto crop=crops.first().toObject();
    crop.insert(QStringLiteral("fertilizerApplied"),false);crops[0]=crop;
    w.insert(QStringLiteral("crops"),crops);root.insert(QStringLiteral("world"),w);
    QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QJsonDocument(root).toJson());file.close();
    GameSaveData unchanged=loaded;
    QVERIFY(!SaveManager::load(path,&loaded,&error));
    QCOMPARE(loaded.gameMinutes,unchanged.gameMinutes);
    QVERIFY(loaded.world.crops.first().fertilizerApplied);
    root.insert(QStringLiteral("schemaVersion"),1);
    QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));file.write(QJsonDocument(root).toJson());file.close();
    QVERIFY(!SaveManager::load(path,&loaded,&error));
}

QTEST_APPLESS_MAIN(CoreTests)

#include "core_tests.moc"
