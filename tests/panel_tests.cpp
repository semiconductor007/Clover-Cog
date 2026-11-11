#include "inventorydialog.h"
#include "marketdialog.h"
#include "seedshopdialog.h"

#include <QApplication>
#include <QListWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QtTest>

class PanelTests : public QObject {
    Q_OBJECT
private slots:
    void bagSelectionAndRefresh();
    void bagMouseAndCloseKeys();
    void shopOrders();
    void marketQuoteAndSale();
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

QTEST_MAIN(PanelTests)
#include "panel_tests.moc"
