#include "economy/economyservice.h"
#include "ui/itembar.h"
#include "map/mapwidget.h"
#include "player/rpg.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QPainter>
#include <QSignalSpy>
#include <QtTest>

class FarmControlsTests : public QObject {
    Q_OBJECT

    // Match the production event path, including the scaled graphics proxy.
    QGraphicsScene scene;
    QGraphicsView view{&scene};
    MapWidget *map = nullptr;
    ImageMovementWidget *player = nullptr;
    ItemBar *bar = nullptr;
    const QPoint plot{24, 12}; // grass next to the starting position in map6

    QPoint tileCenter(QPoint tile) const {
        return QPoint(tile.x() * map->tileWidth() + map->tileWidth() / 2 - map->offsetX(),
                      tile.y() * map->tileHeight() + map->tileHeight() / 2 - map->offsetY());
    }

    void click(QPoint mapPoint, Qt::MouseButton button = Qt::LeftButton) {
        QTest::mouseClick(view.viewport(), button, Qt::NoModifier, view.mapFromScene(mapPoint));
    }

    void scroll(QPoint mapPoint, int angle, int pixels = 0) {
        const QPoint pos = view.mapFromScene(mapPoint);
        QWheelEvent event(pos, view.viewport()->mapToGlobal(pos), QPoint(0, pixels), QPoint(0, angle),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(view.viewport(), &event);
    }

private slots:
    void init() {
        auto *canvas = new QWidget;
        canvas->setFixedSize(520, 320);
        map = new MapWidget(canvas);
        map->setGeometry(canvas->rect());
        QVERIFY(map->loadMap(QStringLiteral(":/map/map6.tmj")));
        player = new ImageMovementWidget(map, map);
        player->setInitialPosition(QPoint(400, 192));
        bar = new ItemBar(map);
        bar->resize(bar->sizeHint());
        bar->move((map->width() - bar->width()) / 2, map->height() - bar->height() - 8);
        connect(bar, &ItemBar::itemSelected, player, &ImageMovementWidget::setCurrentTool);
        connect(map, &MapWidget::mapClicked, player, &ImageMovementWidget::useCurrentToolAt);
        scene.setSceneRect(canvas->rect());
        scene.addWidget(canvas);
        view.setFrameShape(QFrame::NoFrame);
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.resetTransform();
        view.scale(1.5, 1.5);
        view.resize(820, 510);
        view.show();
        view.activateWindow();
        view.setFocus();
        QVERIFY(QTest::qWaitForWindowActive(&view));
    }

    void cleanup() {
        scene.clear();
    }

    void toolbarSelection() {
        QSignalSpy selected(bar, &ItemBar::itemSelected);
        QSignalSpy mapClicks(map, &MapWidget::mapClicked);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        const QPoint row = bar->pos() + QPoint(0, bar->height() - 12);
        const std::array expected{ToolType::EmptyHand, ToolType::Hoe, ToolType::WateringCan,
                                  ToolType::Wheat, ToolType::Potato, ToolType::Carrot,
                                  ToolType::Beet, ToolType::Fertilizer};
        QCOMPARE(ToolbarSlotCount, int(expected.size()));
        for (int i = 0; i < ToolbarSlotCount; ++i) {
            click(row + QPoint((2 * i + 1) * bar->width() / (2 * ToolbarSlotCount), 0));
            QCOMPARE(bar->selectedTool(), expected[i]);
            QCOMPARE(player->getCurrentTool().type, expected[i]);
            QVERIFY(!bar->accessibleDescription().isEmpty());
        }
        QCOMPARE(selected.count(), ToolbarSlotCount - 1);
        QCOMPARE(mapClicks.count(), 0);
        for (const ToolType tool : expected) {
            scroll(tileCenter(plot), -120);  // anywhere on the map
            QCOMPARE(player->getCurrentTool().type, tool);
        }
        for (const ToolType tool : expected) {
            scroll(tileCenter(plot), 0, -15);
            scroll(tileCenter(plot), 0, -15); // two small touchpad deltas make one slot
            QCOMPARE(player->getCurrentTool().type, tool);
        }
        scroll(tileCenter(plot), -120);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        scroll(row + QPoint(10, 0), 120);
        QCOMPARE(bar->selectedTool(), ToolType::Fertilizer);
        scroll(tileCenter(plot), 0);
        QCOMPARE(bar->selectedTool(), ToolType::Fertilizer);
        bar->selectTool(ToolType::Carrot);  // inventory selection
        QCOMPARE(player->getCurrentTool().type, ToolType::Carrot);
        QCOMPARE(bar->accessibleDescription(), QStringLiteral("胡萝卜种子"));
        const auto count = selected.count();
        bar->selectTool(ToolType::Carrot);
        bar->selectTool(static_cast<ToolType>(-1));
        QCOMPARE(selected.count(), count);
    }

    void hoeExplainsItsTarget() {
        bar->selectTool(ToolType::Hoe);
        QSignalSpy feedback(map, &MapWidget::farmingFeedback);
        QSignalSpy changes(map, &MapWidget::worldChanged);
        click(tileCenter(plot));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("已翻土，可以播种"));
        QVERIFY(map->isPlantable(plot));
        QCOMPARE(changes.count(), 1);

        const QPoint readyField(11, 8);
        QVERIFY(map->isPlantable(readyField));
        click(tileCenter(readyField));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("这里可以直接播种"));

        const QPoint road(26, 13);
        click(tileCenter(road));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("道路不能耕种，请选草地"));
        QVERIFY(!map->isPlantable(road));
        QCOMPARE(changes.count(), 1);

        const QPoint besideChest(31, 21);
        map->setViewportCenter(besideChest.x() * map->tileWidth(), besideChest.y() * map->tileHeight());
        map->setStorageArea(QRect(besideChest, QSize(1, 1)));
        click(tileCenter(besideChest));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("互动区域不能耕种，请选草地"));
        QVERIFY(!map->isPlantable(besideChest));
        // A reserved interaction area also takes precedence over otherwise usable field soil.
        map->setStorageArea(QRect(plot, QSize(1, 1)));
        QVERIFY(!map->plantSeed(plot, CropType::Wheat));
        QVERIFY(!map->waterTile(plot));
        map->setStorageArea({});
        QVERIFY(map->plantSeed(plot, CropType::Wheat));
        click(tileCenter(plot));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("这里已经种有作物"));
        QCOMPARE(changes.count(), 2);
    }

    void scaledMapBoundaries_data() {
        QTest::addColumn<qreal>("scale");
        QTest::newRow("reduced") << 0.75;
        QTest::newRow("integer") << 2.0;
    }

    void scaledMapBoundaries() {
        QFETCH(qreal, scale);
        view.resetTransform();
        view.scale(scale, scale);
        QVERIFY(map->offsetX() > 0);
        QSignalSpy clicks(map, &MapWidget::mapClicked);
        click(tileCenter(plot), Qt::RightButton);
        QCOMPARE(clicks.count(), 0);
        bar->selectTool(ToolType::Hoe);
        click(tileCenter(plot));
        QCOMPARE(clicks.count(), 1);
        QCOMPARE(map->mapPointToTile(clicks.first().first().toPoint()), plot);
        QVERIFY(map->isPlantable(plot));
        for (const auto point : {QPoint(-1, 0), QPoint(0, -1),
                                 QPoint(map->width(), 0), QPoint(0, map->height())})
            QCOMPARE(map->mapPointToTile(point), QPoint(-1, -1));

        // A fractional position before the left edge must never round into column zero.
        const QPointF outside(-0.1, 10);
        QMouseEvent press(QEvent::MouseButtonPress, outside, outside, outside,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(map, &press);
        QCOMPARE(clicks.count(), 1);
        map->setViewportCenter(0, 0);
        QCOMPARE(map->mapPointToTile(QPoint(0, 0)), QPoint(0, 0));
        map->setViewportCenter(INT_MAX, INT_MAX);
        QCOMPARE(map->mapPointToTile(map->rect().bottomRight()),
                 QPoint(map->mapWidth() - 1, map->mapHeight() - 1));
    }

    void plantFromToolbar_data() {
        QTest::addColumn<int>("cropId");
        QTest::addColumn<int>("day");
        QTest::addColumn<bool>("inSeason");
        for (CropType crop : CropCatalog::cropTypes()) {
            const auto &definition = CropCatalog::definition(crop);
            QTest::newRow(qPrintable(definition.id)) << static_cast<int>(crop)
                << (crop == CropType::Beet ? 2 * SeasonCalendar::DaysPerSeason : 0) << true;
        }
        QTest::newRow("beet-in-spring") << static_cast<int>(CropType::Beet) << 0 << false;
    }

    void plantFromToolbar() {
        QFETCH(int, cropId);
        QFETCH(int, day);
        QFETCH(bool, inSeason);
        const auto crop = static_cast<CropType>(cropId);
        EconomyService economy;
        auto state = economy.state();
        state.seedCounts[cropId] = 1;
        QVERIFY(economy.restore(state));
        map->setTimeReference([day] { return day * 1440LL; });
        map->setPlantHandler([&](CropType type, const std::function<bool()> &plant) {
            return economy.plantWithSeed(type, plant);
        });
        QSignalSpy outOfSeason(map, &MapWidget::plantingOutOfSeason);
        bar->selectTool(toolForSeed(crop));
        click(tileCenter(plot));  // unprepared ground
        QCOMPARE(economy.seedCount(crop), 1);
        bar->selectTool(ToolType::Hoe);
        click(tileCenter(plot));
        bar->selectTool(toolForSeed(crop));
        click(tileCenter(plot));
        QCOMPARE(economy.seedCount(crop), inSeason ? 0 : 1);
        const auto crops = map->worldSnapshot().crops;
        if (inSeason) {
            QCOMPARE(crops.size(), 1);
            QCOMPARE(crops.first().type, crop);
            QCOMPARE(crops.first().tilePos, plot);
            const QPoint nextPlot(24, 16);
            QVERIFY(map->cultivateTile(nextPlot));
            QSignalSpy blocked(map, &MapWidget::plantingBlocked);
            click(tileCenter(nextPlot));
            QCOMPARE(blocked.count(), 1);
            QVERIFY(map->isPlantable(nextPlot));
        } else {
            QVERIFY(crops.isEmpty());
            QCOMPARE(outOfSeason.count(), 1);
        }
    }

    void singleClickHarvest() {
        EconomyService economy(1);
        int plantAttempts = 0;
        map->setPlantHandler([&](CropType type, const std::function<bool()> &plant) {
            ++plantAttempts;
            return economy.plantWithSeed(type, plant);
        });
        map->setHarvestHandler([&](CropType type, int amount) { return economy.collectHarvest(type, amount); });
        QVERIFY(map->cultivateTile(plot));
        QVERIFY(map->plantSeed(plot, CropType::Wheat));
        auto snapshot = map->worldSnapshot();
        snapshot.crops.first().growthDays = CropCatalog::definition(CropType::Wheat).growthDays;
        QVERIFY(map->restoreWorld(snapshot));
        QSignalSpy mapClicks(map, &MapWidget::mapClicked);
        QSignalSpy harvested(map, &MapWidget::cropHarvested);
        QSignalSpy blocked(map, &MapWidget::harvestBlocked);
        bar->selectTool(ToolType::Wheat);
        const int seeds = economy.seedCount(CropType::Wheat);
        QVERIFY(economy.collectHarvest(CropType::Potato, 1)); // fill the bag
        click(tileCenter(plot));
        QCOMPARE(blocked.count(), 1);
        QCOMPARE(map->worldSnapshot().crops.size(), 1);
        QVERIFY(economy.sellAll({}, QRect(0, 0, 1, 1)).success);
        click(tileCenter(plot));
        QCOMPARE(harvested.count(), 1);
        QVERIFY(map->worldSnapshot().crops.isEmpty());
        QCOMPARE(economy.inventory().count(CropType::Wheat), 1);
        QCOMPARE(economy.seedCount(CropType::Wheat), seeds);
        QCOMPARE(plantAttempts, 1);
        QCOMPARE(mapClicks.count(), 0);
    }

    void fertilizerClickLeavesMatureCropInPlace() {
        QVERIFY(map->cultivateTile(plot));
        QVERIFY(map->plantSeed(plot, CropType::Wheat));
        const int matureDays = CropCatalog::definition(CropType::Wheat).growthDays;
        for (int day = 0; day < matureDays; ++day) {
            QVERIFY(map->waterTile(plot));
            map->advanceDay(day);
        }

        int stock = 1;
        int attempts = 0;
        map->setFertilizeHandler([&](const std::function<bool()> &apply) {
            ++attempts;
            if (stock == 0 || !apply()) return false;
            --stock;
            return true;
        });
        QSignalSpy harvested(map, &MapWidget::cropHarvested);
        QSignalSpy changed(map, &MapWidget::worldChanged);
        QSignalSpy feedback(map, &MapWidget::farmingFeedback);
        bar->selectTool(ToolType::Fertilizer);
        click(tileCenter(plot));
        QCOMPARE(harvested.count(), 0);
        QCOMPARE(attempts, 1);
        QCOMPARE(stock, 1);
        QCOMPARE(changed.count(), 0);
        QCOMPARE(feedback.count(), 1);
        QVERIFY(feedback.first().first().toString().contains(QStringLiteral("已成熟")));
        const auto crops = map->worldSnapshot().crops;
        QCOMPARE(crops.size(), 1);
        QCOMPARE(crops.first().tilePos, plot);
        QCOMPARE(crops.first().type, CropType::Wheat);
        QCOMPARE(crops.first().growthDays, matureDays);
        QVERIFY(!crops.first().fertilizerApplied);
        QVERIFY(!crops.first().fertilizerBonusPending);

        bar->selectTool(ToolType::EmptyHand);
        click(tileCenter(plot));
        QCOMPARE(harvested.count(), 1);
        QCOMPARE(attempts, 1);
        QVERIFY(map->worldSnapshot().crops.isEmpty());
    }

    void fertilizerAndWater() {
        int stock = 2;
        int effects = 0;
        map->setFertilizeHandler([&](const std::function<bool()> &apply) {
            if (!stock) return false;
            ++effects;
            if (!apply()) return false;
            --stock;
            return true;
        });
        QSignalSpy feedback(map, &MapWidget::farmingFeedback);
        QSignalSpy watered(map, &MapWidget::tileWatered);
        bar->selectTool(ToolType::Fertilizer);
        click(tileCenter(plot));
        QCOMPARE(stock, 2);
        QVERIFY(feedback.last().first().toString().contains(QStringLiteral("作物")));
        QVERIFY(map->cultivateTile(plot));
        QVERIFY(map->plantSeed(plot, CropType::Wheat));
        feedback.clear();
        stock = 0;
        click(tileCenter(plot));
        QVERIFY(feedback.last().first().toString().contains(QStringLiteral("不足")));
        QCOMPARE(effects, 1);
        QVERIFY(!map->worldSnapshot().crops.first().fertilizerApplied);
        stock = 2;
        QSignalSpy changes(map, &MapWidget::worldChanged);
        click(tileCenter(plot));
        click(tileCenter(plot));
        QCOMPARE(changes.count(), 1);
        QCOMPARE(stock, 1);
        QCOMPARE(effects, 3);
        QCOMPARE(feedback.count(), 3);
        QVERIFY(feedback.last().first().toString().contains(QStringLiteral("已经")));
        QVERIFY(map->worldSnapshot().crops.first().fertilizerBonusPending);
        bar->selectTool(ToolType::WateringCan);
        click(tileCenter(plot));
        click(tileCenter(plot));
        QCOMPARE(watered.count(), 1);
        QCOMPARE(map->advanceDay(0).fertilizerBonusesUsed, 1);
    }

    void abortedFertilizingDiscardsEffect() {
        QVERIFY(map->cultivateTile(plot));
        QVERIFY(map->plantSeed(plot, CropType::Wheat));
        bool first = false;
        bool second = true;
        map->setFertilizeHandler([&](const std::function<bool()> &apply) {
            first = apply();
            second = apply();
            return false;
        });
        QSignalSpy changes(map, &MapWidget::worldChanged);
        bar->selectTool(ToolType::Fertilizer);
        click(tileCenter(plot));
        QVERIFY(first);
        QVERIFY(!second);
        QCOMPARE(changes.count(), 0);
        QVERIFY(!map->worldSnapshot().crops.first().fertilizerApplied);
    }

    void panelLocksAndRestoresInput() {
        QSignalSpy inventory(player, &ImageMovementWidget::inventoryRequested);
        QSignalSpy interactions(player, &ImageMovementWidget::interactionRequested);
        QSignalSpy clicks(map, &MapWidget::mapClicked);
        const auto start = player->absolutePosition();
        // Selecting with the mouse must preserve gameplay keyboard focus.
        click(bar->pos() + QPoint(3, bar->height() - 5));
        QTest::keyClick(&view, Qt::Key_Tab);
        QCOMPARE(inventory.count(), 1);
        QKeyEvent repeat(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, {}, true);
        QApplication::sendEvent(&view, &repeat);
        QCOMPARE(inventory.count(), 1);
        QTest::keyPress(&view, Qt::Key_D);
        QTRY_VERIFY(player->absolutePosition().x() > start.x());
        map->setInputBlocked(true);
        player->setInputBlocked(true);
        const auto paused = player->absolutePosition();
        QTest::keyClick(&view, Qt::Key_E);
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::keyPress(&view, Qt::Key_S);
        click(tileCenter(plot));
        scroll(tileCenter(plot), -120);
        QCOMPARE(clicks.count(), 0);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        QCOMPARE(interactions.count(), 0);
        QCOMPARE(inventory.count(), 1);
        // Selecting in a panel still equips the tool during the lock.
        bar->selectTool(ToolType::Carrot);
        map->setInputBlocked(false);
        player->setInputBlocked(false);
        QTest::qWait(220);
        QCOMPARE(player->absolutePosition(), paused);
        QCOMPARE(player->getCurrentTool().type, ToolType::Carrot);
        QTest::keyRelease(&view, Qt::Key_S);
        QTest::keyRelease(&view, Qt::Key_D);
        QTest::keyClick(&view, Qt::Key_E);
        QCOMPARE(interactions.count(), 1);
        QTest::keyPress(&view, Qt::Key_A);
        QTRY_VERIFY(player->absolutePosition().x() < paused.x());
        QTest::keyRelease(&view, Qt::Key_A);
    }

    void treesRemainScenery() {
        const QPoint tree(46, 21);
        const QPoint standing = tree - QPoint(0, 1);
        player->setInitialPosition(standing * 16 + QPoint(8, 8) -
                                   QPoint(player->width() / 2, player->height() / 2));
        QCOMPARE(player->currentTilePosition(), standing);
        QVERIFY(map->hasTreeAt(tree));
        const int collision = map->collisionLayer().at(tree.y()).at(tree.x());
        QVERIFY(collision != 0);
        // Even a stale selection from a previous UI cannot equip the retired axe.
        bar->selectTool(ToolType::Axe);
        player->setCurrentTool(static_cast<int>(ToolType::Axe));
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        QCOMPARE(player->getCurrentTool().type, ToolType::EmptyHand);
        for (int i = 0; i < 5; ++i) {
            click(player->geometry().center());
            click(tileCenter(tree));
        }
        QVERIFY(map->hasTreeAt(tree));
        QCOMPARE(map->collisionLayer().at(tree.y()).at(tree.x()), collision);
    }

    void walkThroughDoorAndBack() {
        const QRect bedside(25, 8, 2, 2);
        QTest::keyPress(&view, Qt::Key_W);
        QTRY_VERIFY(bedside.contains(player->currentTilePosition()));
        QTest::keyRelease(&view, Qt::Key_W);
        QSignalSpy interactions(player, &ImageMovementWidget::interactionRequested);
        QTest::keyClick(&view, Qt::Key_E);
        QCOMPARE(interactions.count(), 1);
        QVERIFY(bedside.contains(interactions.first().first().toPoint()));
        QTest::keyPress(&view, Qt::Key_S);
        QTRY_VERIFY(player->currentTilePosition().y() >= 13);
        QTest::keyRelease(&view, Qt::Key_S);
    }

    void restorePlayerAroundNewObstacles() {
        const QPoint spriteCenter(player->width() / 2, player->height() / 2);
        const int tileWidth = map->tileWidth(), tileHeight = map->tileHeight();
        const QPoint crateTopLeft(31 * tileWidth, 20 * tileHeight);
        const QPoint leftCellCenter = crateTopLeft + QPoint(-tileWidth / 2, tileHeight / 2);

        const QPoint clear = leftCellCenter - spriteCenter + QPoint(2, 1);
        QVERIFY(player->restorePosition(clear));
        QCOMPARE(player->absolutePosition(), clear); // saved sub-tile precision is preserved

        // The player's center is left of the crate, but the right part of its body overlaps.
        const QPoint intersecting = crateTopLeft + QPoint(-2, tileHeight / 2) - spriteCenter;
        QVERIFY(player->restorePosition(intersecting));
        QCOMPARE(player->absolutePosition(), leftCellCenter - spriteCenter);
        QCOMPARE(player->currentTilePosition(), QPoint(30, 20));
        QVERIFY(!player->restorePosition(QPoint(-1, -1)));
        QCOMPARE(player->absolutePosition(), leftCellCenter - spriteCenter);
    }

    void collisionAndPlayerBounds() {
        QVector<QVector<int>> cells{{0, 0}, {0, 1}};
        CollisionDetector collision(&cells, 16, 16);
        QVERIFY(!collision.isColliding(QRect(8, 8, 8, 8)));
        QVERIFY(collision.isColliding(QRect(9, 9, 8, 8)));
        player->setInitialPosition(QPoint(-3, -12));
        QCOMPARE(player->absolutePosition(), QPoint(0, 0));
        player->setInitialPosition(QPoint(INT_MAX, INT_MAX));
        QCOMPARE(player->absolutePosition(),
                 QPoint(map->mapWidth() * map->tileWidth() - player->width(),
                        map->mapHeight() * map->tileHeight() - player->height()));
    }

    void seasonAppearanceKeepsWorldAndSources() {
        QVERIFY(map->cultivateTile(plot));
        QVERIFY(map->plantSeed(plot, CropType::Potato));
        auto display = map->originalTilesets();
        const auto sourceKey = display.first().texture.cacheKey();
        display.first().texture.fill(Qt::cyan);
        QPoint camera;
        bool sawCrop = false;
        map->setSeasonAppearance(display, [&](QPainter &, const QPoint &offset, const WorldState &world) {
            camera = offset;
            sawCrop = world.cropGidAt(plot) == CropCatalog::stageGid(CropType::Potato, 0);
        });
        QVERIFY(!map->grab().isNull());
        QCOMPARE(camera, QPoint(map->offsetX(), map->offsetY()));
        QVERIFY(sawCrop);
        QCOMPARE(map->originalTilesets().first().texture.cacheKey(), sourceKey);
        map->setSeasonAppearance({}, {});
        QCOMPARE(map->worldSnapshot().crops.first().type, CropType::Potato);
    }
};

QTEST_MAIN(FarmControlsTests)
#include "farm_controls_tests.moc"
