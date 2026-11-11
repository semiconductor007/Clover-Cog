#include <QtTest>
#include <limits>
#include "economyservice.h"
#include "worldstate.h"

namespace {
const QRect market(28, 18, 2, 2);
const QPoint atMarket(28, 18);
constexpr int maxCount = std::numeric_limits<int>::max();
}

class EconomyTests : public QObject {
    Q_OBJECT

private slots:
    void approvedPricesAndRevenue();
    void fullBagStillAcceptsSupplies();
    void onlySuccessfulFertilizationConsumesStock();
    void itemActionsCannotReenter();
    void invalidAndOverflowingPurchasesPreserveAssets();
    void saleOverflowPreservesWholeBag();
};

void EconomyTests::approvedPricesAndRevenue() {
    EconomyService economy;
    QVERIFY(economy.collectHarvest(CropType::Wheat, 6));
    QCOMPARE(economy.sellAll(atMarket, market).coinsEarned, 30);

    QCOMPARE(economy.buySeeds(CropType::Wheat, 1).coinsSpent, 2);
    QCOMPARE(economy.buySeeds(CropType::Potato, 1).coinsSpent, 4);
    QCOMPARE(economy.buySeeds(CropType::Carrot, 1).coinsSpent, 2);
    QCOMPARE(economy.buySeeds(CropType::Beet, 1).coinsSpent, 3);
    QCOMPARE(economy.coins(), 19);

    QVERIFY(economy.collectHarvest(CropType::Wheat, 1));
    QVERIFY(economy.collectHarvest(CropType::Potato, 3));
    QVERIFY(economy.collectHarvest(CropType::Carrot, 2));
    QVERIFY(economy.collectHarvest(CropType::Beet, 2));
    QVERIFY(!economy.sellAll(QPoint(0, 0), market).success);
    QCOMPARE(economy.inventory().usedCapacity(), 8);
    const SaleResult sale = economy.sellAll(atMarket, market);
    QCOMPARE(sale.itemsSold, 8);
    QCOMPARE(sale.coinsEarned, 28);
    QCOMPARE(economy.coins(), 47);
    QVERIFY(!economy.sellAll(atMarket, market).success);
    QCOMPARE(economy.coins(), 47);
}

void EconomyTests::fullBagStillAcceptsSupplies() {
    EconomyService economy;
    QVERIFY(economy.collectHarvest(CropType::Wheat, 6));
    QCOMPARE(economy.sellAll(atMarket, market).coinsEarned, 30);
    QVERIFY(economy.collectHarvest(CropType::Beet, 11));
    QVERIFY(!economy.collectHarvest(CropType::Potato, 3));
    QCOMPARE(economy.inventory().usedCapacity(), 11);
    QVERIFY(!economy.collectHarvest(static_cast<CropType>(999), 1));
    QVERIFY(economy.collectHarvest(CropType::Wheat, 1));
    QCOMPARE(economy.buySeeds(CropType::Carrot, 5).coinsSpent, 10);
    QCOMPARE(economy.buyFertilizer(5).coinsSpent, 5);
    QCOMPARE(economy.coins(), 15);
    QCOMPARE(economy.inventory().usedCapacity(), 12);
    QCOMPARE(economy.seedCount(CropType::Carrot), 5);
    QCOMPARE(economy.fertilizerCount(), 5);
}

void EconomyTests::onlySuccessfulFertilizationConsumesStock() {
    MapData map;
    map.mapWidth = 2;
    map.mapHeight = 1;
    map.cultivatedLayer = {{135, 135}};
    WorldState world;
    world.initialize(map);
    const QPoint field(0, 0);
    EconomyService economy;
    QVERIFY(economy.plantWithSeed(CropType::Wheat, [&] {
        return world.plantSeed(field, CropType::Wheat, 0);
    }));
    const auto apply = [&] { return world.fertilize(field).success; };
    QVERIFY(!economy.fertilizeWithItem(apply));
    QVERIFY(!world.isFertilized(field));

    QVERIFY(economy.collectHarvest(CropType::Wheat, 1));
    QCOMPARE(economy.sellAll(atMarket, market).coinsEarned, 5);
    QCOMPARE(economy.buyFertilizer(2).coinsSpent, 2);
    QVERIFY(!economy.fertilizeWithItem([&] {
        return world.fertilize(QPoint(1, 0)).success;
    }));
    QCOMPARE(economy.fertilizerCount(), 2);
    QVERIFY(economy.fertilizeWithItem(apply));
    QVERIFY(world.hasPendingFertilizerBonus(field));
    QCOMPARE(economy.fertilizerCount(), 1);
    QVERIFY(!economy.fertilizeWithItem(apply));
    QCOMPARE(economy.fertilizerCount(), 1);
    QCOMPARE(economy.coins(), 3);

    EconomyService restored;
    QVERIFY(restored.restore(economy.state()));
    QCOMPARE(restored.fertilizerCount(), 1);
    auto invalid = restored.state();
    invalid.fertilizerCount = -1;
    invalid.coins = 0;
    QVERIFY(!restored.restore(invalid));
    QCOMPARE(restored.fertilizerCount(), 1);
    QCOMPARE(restored.coins(), 3);
}

void EconomyTests::itemActionsCannotReenter() {
    EconomyService economy;
    auto state = economy.state();
    state.fertilizerCount = 1;
    QVERIFY(economy.restore(state));
    bool nestedSuccess = true;
    int applications = 0;
    QVERIFY(economy.fertilizeWithItem([&] {
        ++applications;
        nestedSuccess = economy.fertilizeWithItem([&] {
            ++applications;
            return true;
        });
        return true;
    }));
    QVERIFY(!nestedSuccess);
    QCOMPARE(applications, 1);
    QCOMPARE(economy.fertilizerCount(), 0);
}

void EconomyTests::invalidAndOverflowingPurchasesPreserveAssets() {
    EconomyService economy;
    QCOMPARE(economy.buyFertilizer(1).failure, SeedPurchaseFailure::InsufficientCoins);
    QCOMPARE(economy.fertilizerCount(), 0);
    QCOMPARE(economy.buyFertilizer(-1).failure, SeedPurchaseFailure::InvalidRequest);
    QCOMPARE(economy.buySeeds(CropType::None, 1).failure, SeedPurchaseFailure::InvalidRequest);

    auto state = economy.state();
    state.coins = maxCount;
    state.fertilizerCount = maxCount - 1;
    state.seedCounts.insert(static_cast<int>(CropType::Beet), maxCount);
    QVERIFY(economy.restore(state));
    QVERIFY(!economy.buyFertilizer(2).success);
    QVERIFY(!economy.buySeeds(CropType::Beet, 1).success);
    QVERIFY(!economy.buySeeds(CropType::Carrot, maxCount).success);
    QCOMPARE(economy.coins(), maxCount);
    QCOMPARE(economy.fertilizerCount(), maxCount - 1);
    QCOMPARE(economy.seedCount(CropType::Beet), maxCount);
    QCOMPARE(economy.seedCount(CropType::Carrot), 0);
    QVERIFY(economy.buyFertilizer(1).success);
    QCOMPARE(economy.fertilizerCount(), maxCount);
    QCOMPARE(economy.coins(), maxCount - 1);
}

void EconomyTests::saleOverflowPreservesWholeBag() {
    EconomyService economy;
    auto state = economy.state();
    state.coins = maxCount - 5;
    QVERIFY(economy.restore(state));
    QVERIFY(economy.collectHarvest(CropType::Wheat, 1));
    QVERIFY(economy.collectHarvest(CropType::Beet, 1));
    QVERIFY(!economy.sellAll(atMarket, market).success);
    QCOMPARE(economy.inventory().count(CropType::Wheat), 1);
    QCOMPARE(economy.inventory().count(CropType::Beet), 1);
    QCOMPARE(economy.coins(), maxCount - 5);

    state.coins = 0;
    state.inventoryCapacity = maxCount;
    state.inventoryCounts.insert(static_cast<int>(CropType::Wheat), maxCount);
    QVERIFY(economy.restore(state));
    QVERIFY(!economy.sellAll(atMarket, market).success);
    QCOMPARE(economy.inventory().count(CropType::Wheat), maxCount);
    QCOMPARE(economy.coins(), 0);
}

QTEST_APPLESS_MAIN(EconomyTests)
#include "economy_tests.moc"
