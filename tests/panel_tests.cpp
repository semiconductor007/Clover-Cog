#include "ui/inventorydialog.h"
#include "ui/slotgrid.h"
#include "ui/marketdialog.h"
#include "ui/seedshopdialog.h"
#include "ui/storagedialog.h"

#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QListWidget>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QtTest>

class PanelTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
#ifdef Q_OS_WIN
        const int font = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/msyh.ttc"));
        const auto families = QFontDatabase::applicationFontFamilies(font);
        if (qEnvironmentVariable("QT_QPA_PLATFORM") == QStringLiteral("offscreen") && !families.isEmpty())
            QApplication::setFont(QFont(families.first(), 10));
#endif
    }
    void bagSelectionAndRefresh();
    void bagMouseAndCloseKeys();
    void bagToolsAndQuantitiesStayUnchanged();
    void shopOrders();
    void marketQuoteAndSale();
    void storageTransfersAndFailureFeedback();
    void chestDragAndCancel();
};

void PanelTests::bagToolsAndQuantitiesStayUnchanged() {
    EconomyService economy;
    auto state=economy.state(); state.coins=123; state.inventoryCounts[1]=12; state.fertilizerCount=5;
    for (CropType crop : CropCatalog::cropTypes()) state.seedCounts[static_cast<int>(crop)]=9876;
    QVERIFY(economy.restore(state));
    InventoryDialog bag; bag.setState(economy,ToolType::Hoe);
    QSignalSpy selected(&bag,&InventoryDialog::toolRequested);
    bag.show(); QVERIFY(QTest::qWaitForWindowExposed(&bag));
    auto *grid=dynamic_cast<SlotGrid *>(bag.findChild<QWidget *>("bagSlots")); QVERIFY(grid);
    QCOMPARE(grid->cells().size(),21);
    QVERIFY(!bag.findChild<QLabel *>("panelSummary")->isVisible());
    QVERIFY(!bag.findChild<QPushButton *>("bagTool_0"));
    QDir().mkpath("captures");
    QVERIFY(bag.grab().save("captures/inventory-grid.png"));
    EconomyService preview; bag.setState(preview,ToolType::Wheat);
    QCoreApplication::processEvents(); QVERIFY(bag.grab().save("captures/inventory-preview.png"));
    bag.setState(economy,ToolType::Hoe);
    QTest::mouseClick(grid,Qt::LeftButton,{},grid->cellRect(20).center());
    QVERIFY(bag.isVisible()); QCOMPARE(selected.count(),0);
    QTest::keyClick(grid,Qt::Key_E);
    for (int index : {0,1}) {
        bag.show(); QTest::mouseClick(grid,Qt::LeftButton,{},grid->cellRect(index).center());
        QCOMPARE(selected.last().first().value<ToolType>(),index==0 ? ToolType::Hoe : ToolType::WateringCan);
        QVERIFY(!bag.isVisible());
    }
    QCOMPARE(economy.state().inventoryCounts,state.inventoryCounts);
    QCOMPARE(economy.state().seedCounts,state.seedCounts);
    QCOMPARE(economy.coins(),123); QCOMPARE(economy.fertilizerCount(),5);
}

void PanelTests::bagSelectionAndRefresh() {
    EconomyService economy; auto state=economy.state(); state.seedCounts[3]=2; QVERIFY(economy.restore(state));
    InventoryDialog bag; bag.setState(economy,ToolType::Carrot,6);
    QSignalSpy chosen(&bag,&InventoryDialog::toolRequested);
    bag.show(); QVERIFY(QTest::qWaitForWindowExposed(&bag));
    auto *grid=dynamic_cast<SlotGrid *>(bag.findChild<QWidget *>("bagSlots")); QVERIFY(grid);
    int index=-1; for (int i=0;i<21;++i) if(grid->cells()[i].item==StackItem::seed(CropType::Carrot)) index=i;
    QVERIFY(index>=0 && index<7);
    QVERIFY(economy.plantWithSeed(CropType::Carrot,[]{return true;}));
    bag.setState(economy,ToolType::Carrot,7);
    QVERIFY(bag.findChild<QLabel *>("bagDate")->text().contains(QStringLiteral("夏")));
    QCOMPARE(grid->cells()[index].quantity,1);
    QTest::mouseClick(grid,Qt::LeftButton,{},grid->cellRect(index).center());
    QCOMPARE(chosen.count(),1); QCOMPARE(chosen.first().first().value<ToolType>(),ToolType::Carrot);
    QCOMPARE(economy.seedCount(CropType::Carrot),1); QVERIFY(!bag.isVisible());
}

void PanelTests::bagMouseAndCloseKeys() {
    EconomyService economy; InventoryDialog bag; bag.setState(economy,ToolType::EmptyHand);
    auto *grid=dynamic_cast<SlotGrid *>(bag.findChild<QWidget *>("bagSlots")); QVERIFY(grid);
    connect(&bag,&InventoryDialog::stackMoveRequested,&bag,[&](StackMove move){
        QVERIFY(economy.moveStack(move)); bag.setState(economy,ToolType::EmptyHand);
    });
    bag.show(); QVERIFY(QTest::qWaitForWindowExposed(&bag));
    QTest::mouseClick(grid,Qt::RightButton,{},grid->cellRect(2).center());
    QCOMPARE(economy.bagSlots()[2].quantity,5); QCOMPARE(economy.bagSlots()[4].quantity,1);
    QTest::mouseClick(grid,Qt::RightButton,Qt::ShiftModifier,grid->cellRect(2).center());
    QCOMPARE(economy.bagSlots()[2].quantity,2); QCOMPARE(economy.bagSlots()[5].quantity,3);
    QTest::mousePress(grid,Qt::LeftButton,{},grid->cellRect(5).center());
    QTest::mouseMove(grid,grid->cellRect(2).center());
    QTest::mouseRelease(grid,Qt::LeftButton,{},grid->cellRect(2).center());
    QTest::mousePress(grid,Qt::LeftButton,{},grid->cellRect(4).center());
    QTest::mouseMove(grid,grid->cellRect(2).center());
    QTest::mouseRelease(grid,Qt::LeftButton,{},grid->cellRect(2).center());
    QCOMPARE(economy.bagSlots()[2].quantity,6); QCOMPARE(economy.bagSlots()[4].item,0);
    const auto before=economy.state();
    QTest::mousePress(grid,Qt::LeftButton,{},grid->cellRect(2).center());
    QTest::mouseMove(grid,grid->cellRect(10).center());
    QTest::keyClick(grid,Qt::Key_Escape);
    QTest::mouseRelease(grid,Qt::LeftButton,{},grid->cellRect(10).center());
    QCOMPARE(economy.bagSlots(),before.bagSlots);
    for (Qt::Key key : {Qt::Key_E,Qt::Key_Tab,Qt::Key_Escape}) {
        bag.show(); grid->setFocus(); QTest::keyClick(grid,key); QVERIFY(!bag.isVisible());
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
    auto *quantity = market.findChild<QSpinBox *>("sellQuantity_potato");
    auto *sellPotatoes = market.findChild<QPushButton *>("sell_potato");
    QVERIFY(quantity && sellPotatoes);
    quantity->stepUp();
    market.setState(economy);
    QCOMPARE(quantity->value(), 2);
    QCOMPARE(sellPotatoes->text(), QStringLiteral("卖出 · 6 金币"));
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

void PanelTests::chestDragAndCancel() {
    EconomyService economy; auto state=economy.state();state.inventoryCounts[1]=11;state.storage.harvestCounts[1]=2;
    QVERIFY(economy.restore(state)); StorageDialog storage;storage.setState(economy);
    auto *bag=dynamic_cast<SlotGrid *>(storage.findChild<QWidget *>("storageBagSlots"));
    auto *chest=dynamic_cast<SlotGrid *>(storage.findChild<QWidget *>("chestSlots"));
    QVERIFY(bag && chest);
    bool accepted=false;
    connect(&storage,&StorageDialog::stackMoveRequested,&storage,[&](StackMove move){
        accepted=economy.moveStack(move);storage.setState(economy);
    });
    storage.show();QVERIFY(QTest::qWaitForWindowExposed(&storage));
    const auto drag=[](SlotGrid *from,int source,SlotGrid *to,int destination,Qt::MouseButton button) {
        const QPoint target=from->mapFromGlobal(to->mapToGlobal(to->cellRect(destination).center()));
        QTest::mousePress(from,button,{},from->cellRect(source).center());
        QTest::mouseMove(from,target);
        QTest::mouseRelease(from,button,{},target);
    };
    drag(chest,0,bag,10,Qt::RightButton);
    QVERIFY(accepted); QCOMPARE(economy.inventory().usedCapacity(),12);QCOMPARE(economy.storage().harvestCounts.value(1),1);
    const auto before=economy.state();
    accepted=true;drag(chest,0,bag,11,Qt::LeftButton);QVERIFY(!accepted);
    QCOMPARE(economy.bagSlots(),before.bagSlots);QCOMPARE(economy.chestSlots(),before.chestSlots);
    drag(bag,0,chest,5,Qt::LeftButton);QVERIFY(accepted);
    QCOMPARE(economy.chestSlots()[5].item,StackItem::Hoe);
    drag(bag,4,bag,15,Qt::RightButton);QVERIFY(accepted);
    QCOMPARE(economy.bagSlots()[15].item,StackItem::harvest(CropType::Wheat));
    QCOMPARE(economy.inventory().usedCapacity(),12);
    const auto closing=economy.state();
    QTest::mousePress(bag,Qt::LeftButton,{},bag->cellRect(2).center());
    QTest::mouseMove(bag,bag->cellRect(16).center());
    storage.reject();
    QCOMPARE(economy.bagSlots(),closing.bagSlots);QCOMPARE(economy.chestSlots(),closing.chestSlots);
    QVERIFY(EconomyService::layoutMatches(economy.state()));
}

QTEST_MAIN(PanelTests)
#include "panel_tests.moc"
