#include "inventorydialog.h"
#include "marketdialog.h"
#include "seedshopdialog.h"
#include "storagedialog.h"

#include <QApplication>
#include <QListWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QtTest>

class PanelTests : public QObject {
    Q_OBJECT
private slots:
    void bagSelectionAndRefresh();
    void bagMouseAndCloseKeys();
    void shopOrders();
    void marketQuoteAndSale();
    void storageTransfersAndFailureFeedback();
};

void PanelTests::bagSelectionAndRefresh() {
    EconomyService economy;
    EconomyState state = economy.state();
    state.seedCounts[static_cast<int>(CropType::Carrot)] = 2;
    QVERIFY(economy.restore(state));
    InventoryDialog bag;
    bag.setState(economy, ToolType::Carrot, 6);
    QSignalSpy chosen(&bag, &InventoryDialog::toolRequested);
    bag.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bag));
    auto *tabs = bag.findChild<QTabWidget *>("bagCategories");
    auto *seeds = bag.findChild<QListWidget *>("bagSeeds");
    QVERIFY(tabs && seeds);
    QCOMPARE(tabs->currentWidget(), seeds);
    QVERIFY(seeds->currentItem()->text().contains(QStringLiteral("胡萝卜种子    × 2    · 已选用")));
    QVERIFY(!seeds->currentItem()->icon().isNull());
    QVERIFY(economy.plantWithSeed(CropType::Carrot, [] { return true; }));
    bag.setState(economy, ToolType::Carrot, 7);
    QVERIFY(bag.findChild<QLabel *>("bagDate")->text().contains(QStringLiteral("夏")));
    QVERIFY(seeds->currentItem()->text().contains(QStringLiteral("× 1")));
    QTest::keyClick(seeds, Qt::Key_Return);
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(chosen.first().first().value<ToolType>(), ToolType::Carrot);
    QCOMPARE(economy.seedCount(CropType::Carrot), 1);
    QVERIFY(!bag.isVisible());
}

void PanelTests::bagMouseAndCloseKeys() {
    EconomyService economy;
    EconomyState state = economy.state();
    state.fertilizerCount = 5;
    QVERIFY(economy.restore(state));
    InventoryDialog bag;
    QSignalSpy chosen(&bag, &InventoryDialog::toolRequested);
    bag.setState(economy, ToolType::EmptyHand);
    bag.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bag));
    auto *tabs = bag.findChild<QTabWidget *>("bagCategories");
    auto *harvest = bag.findChild<QListWidget *>("bagHarvest");
    auto *fertilizer = bag.findChild<QListWidget *>("bagFertilizer");
    QVERIFY(tabs && harvest && fertilizer);
    auto *empty = harvest->item(harvest->count() - 1);
    QVERIFY(!empty->isHidden());
    QTest::mouseClick(harvest->viewport(), Qt::LeftButton, {}, harvest->visualItemRect(empty).center());
    QCOMPARE(chosen.count(), 0);
    QKeyEvent heldTab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, QString(), true);
    QApplication::sendEvent(harvest, &heldTab);
    QVERIFY(bag.isVisible());
    QTest::keyClick(harvest, Qt::Key_Left);
    QCOMPARE(tabs->currentWidget(), fertilizer);
    auto *item = fertilizer->item(0);
    QTest::mouseClick(fertilizer->viewport(), Qt::LeftButton, {}, fertilizer->visualItemRect(item).center());
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(chosen.first().first().value<ToolType>(), ToolType::Fertilizer);
    QCOMPARE(economy.fertilizerCount(), 5);
    for (Qt::Key key : {Qt::Key_Tab, Qt::Key_Escape}) {
        bag.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bag));
        QTest::keyClick(fertilizer, key);
        QVERIFY(!bag.isVisible());
        QCOMPARE(chosen.count(), 1);
    }
}

void PanelTests::shopOrders() {
    EconomyService economy;
    EconomyState state = economy.state();
    state.coins = 16;
    QVERIFY(economy.restore(state));
    SeedShopDialog shop;
    shop.setState(economy);
    connect(&shop, &SeedShopDialog::purchaseRequested, &shop, [&](CropType type, int quantity) {
        QVERIFY(economy.buySeeds(type, quantity).success);
        shop.setState(economy);
    });
    connect(&shop, &SeedShopDialog::fertilizerPurchaseRequested, &shop, [&](int quantity) {
        QVERIFY(economy.buyFertilizer(quantity).success);
        shop.setState(economy);
    });
    shop.show();
    QVERIFY(QTest::qWaitForWindowExposed(&shop));
    auto *firstButton = shop.findChild<QPushButton *>("buy_wheat_1");
    QVERIFY(firstButton);
    firstButton->setFocus();
    QTest::keyClick(firstButton, Qt::Key_Tab);
    QCOMPARE(shop.focusWidget(), shop.findChild<QPushButton *>("buy_wheat_5"));
    for (CropType crop : CropCatalog::cropTypes()) {
        const QString id = CropCatalog::definition(crop).id;
        auto *buy = shop.findChild<QPushButton *>(QStringLiteral("buy_%1_1").arg(id));
        QVERIFY(buy);
        QTest::mouseClick(buy, Qt::LeftButton);
        auto *owned = shop.findChild<QLabel *>(QStringLiteral("owned_%1").arg(id));
        QVERIFY(owned);
        QCOMPARE(owned->text(), QString::number(economy.seedCount(crop)));
    }
    QCOMPARE(economy.coins(), 5);
    auto *buyFive = shop.findChild<QPushButton *>("buy_fertilizer_5");
    QVERIFY(buyFive);
    buyFive->setFocus();
    QKeyEvent heldEnter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString(), true);
    QApplication::sendEvent(buyFive, &heldEnter);
    QCOMPARE(economy.coins(), 5);
    QTest::keyClick(buyFive, Qt::Key_Return);
    QCOMPARE(economy.coins(), 0);
    QCOMPARE(shop.findChild<QLabel *>("owned_fertilizer")->text(), QStringLiteral("5"));
    QCOMPARE(economy.inventory().usedCapacity(), 0);
    QTest::keyClick(buyFive, Qt::Key_Escape);
    QVERIFY(!shop.isVisible());
}

void PanelTests::marketQuoteAndSale() {
    EconomyService economy;
    for (CropType crop : CropCatalog::cropTypes())
        QVERIFY(economy.collectHarvest(crop, CropCatalog::definition(crop).harvestYield));
    MarketDialog market;
    market.setState(economy);
    auto *total = market.findChild<QLabel *>("saleTotal");
    auto *sell = market.findChild<QPushButton *>("sellAll");
    QVERIFY(total && sell);
    QVERIFY(total->text().contains(QStringLiteral("共 8 份    合计 28 金币")));
    QCOMPARE(market.findChild<QLabel *>("subtotal_potato")->text(), QStringLiteral("9 金币"));
    QSignalSpy sold(&market, &MarketDialog::saleRequested);
    connect(&market, &MarketDialog::saleRequested, &market, [&] {
        QVERIFY(economy.sellAll({1, 1}, {1, 1, 1, 1}).success);
        market.setState(economy);
        market.setMessage(QStringLiteral("出售成功"));
    });
    market.show();
    QVERIFY(QTest::qWaitForWindowExposed(&market));
    QTest::mouseClick(sell, Qt::LeftButton);
    QTest::mouseClick(sell, Qt::LeftButton);
    QCOMPARE(sold.count(), 1);
    QCOMPARE(economy.coins(), 28);
    QVERIFY(!sell->isEnabled());
    QVERIFY(total->text().contains(QStringLiteral("合计 0 金币")));
}

void PanelTests::storageTransfersAndFailureFeedback() {
    EconomyService economy;
    auto state = economy.state();
    const int wheat = static_cast<int>(CropType::Wheat);
    state.inventoryCounts[wheat] = 11;
    state.storage.harvestCounts[wheat] = 2;
    QVERIFY(economy.restore(state));
    StorageDialog storage;
    storage.setState(economy);
    auto *rows = storage.findChild<QTableWidget *>("storageRows");
    auto *one = storage.findChild<QPushButton *>("withdrawOne");
    auto *all = storage.findChild<QPushButton *>("withdrawAll");
    auto *depositOne = storage.findChild<QPushButton *>("depositOne");
    auto *depositAll = storage.findChild<QPushButton *>("depositAll");
    auto *message = storage.findChild<QLabel *>("panelMessage");
    QVERIFY(rows && one && all && depositOne && depositAll && message);
    connect(&storage, &StorageDialog::transferRequested, &storage,
            [&](StorageItemKind kind, CropType crop, StorageDirection direction, int quantity) {
        const auto result = economy.transfer(kind, crop, direction, quantity);
        storage.setState(economy);
        storage.setMessage(result.message);
    });
    QSignalSpy transfers(&storage, &StorageDialog::transferRequested);
    storage.show();
    QVERIFY(QTest::qWaitForWindowExposed(&storage));
    QTest::mouseClick(all, Qt::LeftButton);
    QVERIFY(message->text().contains(QStringLiteral("只剩 1 格")));
    QCOMPARE(rows->item(0, 1)->text(), QStringLiteral("11"));
    QCOMPARE(rows->item(0, 2)->text(), QStringLiteral("2"));
    QTest::mouseClick(one, Qt::LeftButton);
    QCOMPARE(rows->item(0, 1)->text(), QStringLiteral("12"));
    QCOMPARE(rows->item(0, 2)->text(), QStringLiteral("1"));
    QTest::keyClick(depositAll, Qt::Key_Return);
    QCOMPARE(rows->item(0, 2)->text(), QStringLiteral("13"));
    QVERIFY(!depositAll->isEnabled());
    QCOMPARE(transfers.count(), 3);
    QTest::mouseClick(depositAll, Qt::LeftButton);
    QCOMPARE(transfers.count(), 3);

    const auto seeds = rows->findItems(QStringLiteral("小麦种子"), Qt::MatchExactly);
    QCOMPARE(seeds.size(), 1);
    QTest::mouseClick(rows->viewport(), Qt::LeftButton, Qt::NoModifier, rows->visualItemRect(seeds.first()).center());
    QTest::mouseClick(depositOne, Qt::LeftButton);
    QCOMPARE(economy.seedCount(CropType::Wheat), 5);
    QCOMPARE(economy.storage().seedCounts.value(wheat), 1);
    QKeyEvent repeated(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString(), true);
    QApplication::sendEvent(depositOne, &repeated);
    QCOMPARE(transfers.count(), 4);
    QTest::keyClick(depositOne, Qt::Key_Escape);
    QVERIFY(!storage.isVisible());
}

QTEST_MAIN(PanelTests)
#include "panel_tests.moc"
