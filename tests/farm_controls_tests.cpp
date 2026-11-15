#include "economy/economyservice.h"
#include "ui/itembar.h"
#include "map/mapwidget.h"
#include "player/rpg.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QPainter>
#include <QDir>
#include <QFontDatabase>
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

    void standAt(QPoint tile) {
        player->setInitialPosition(QPoint(tile.x() * map->tileWidth() + map->tileWidth() / 2 - player->width() / 2,
                                         tile.y() * map->tileHeight() + map->tileHeight() / 2 - player->height() / 2));
    }

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
    void initTestCase() {
#ifdef Q_OS_WIN
        if (qEnvironmentVariable("QT_QPA_PLATFORM") == QStringLiteral("offscreen")) {
            const int id = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/segoeui.ttf"));
            const auto families = QFontDatabase::applicationFontFamilies(id);
            if (!families.isEmpty()) QApplication::setFont(QFont(families.first(),10));
        }
#endif
    }
    void init() {
        auto *canvas = new QWidget;
        canvas->setFixedSize(520, 320);
        map = new MapWidget(canvas);
        map->setGeometry(canvas->rect());
        QVERIFY(map->loadMap(QStringLiteral(":/map/map6.tmj")));
        player = new ImageMovementWidget(map, map);
        standAt(QPoint(25, 13));
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

    void interactionReach_data() {
        QTest::addColumn<QPoint>("offset");
        QTest::addColumn<int>("action");
        for (int action = 0; action < 5; ++action)
            for (int y = -2; y <= 2; ++y)
                for (int x = -2; x <= 2; ++x)
                    QTest::newRow(qPrintable(QStringLiteral("action%1_%2_%3").arg(action).arg(x).arg(y)))
                        << QPoint(x, y) << action;
    }

    void interactionReach() {
        QFETCH(QPoint, offset);
        QFETCH(int, action);
        EconomyService economy;
        auto state = economy.state();
        state.fertilizerCount = 2;
        QVERIFY(economy.restore(state));
        const ToolType tools[] = {ToolType::Hoe, ToolType::Wheat, ToolType::WateringCan,
                                  ToolType::Fertilizer, ToolType::EmptyHand};
        if (action != 0) QVERIFY(map->cultivateTile(plot));
        if (action >= 3) QVERIFY(map->plantSeed(plot, CropType::Wheat));
        if (action == 4) {
            auto snapshot = map->worldSnapshot();
            snapshot.crops.first().growthDays = CropCatalog::definition(CropType::Wheat).growthDays;
            QVERIFY(map->restoreWorld(snapshot));
        }
        int handlerCalls = 0;
        map->setPlantHandler([&](CropType crop, const std::function<bool()> &apply) {
            ++handlerCalls;
            return economy.plantWithSeed(crop, apply);
        });
        map->setFertilizeHandler([&](const std::function<bool()> &apply) {
            ++handlerCalls;
            return economy.fertilizeWithItem(apply);
        });
        map->setHarvestHandler([&](CropType crop, int count) {
            ++handlerCalls;
            return economy.collectHarvest(crop, count);
        });
        standAt(plot - offset);
        bar->selectTool(tools[action]);
        const auto before = map->worldSnapshot();
        const auto assets = economy.state();
        QSignalSpy changed(map, &MapWidget::worldChanged);
        QSignalSpy feedback(map, &MapWidget::farmingFeedback);
        click(tileCenter(plot));
        const bool allowed = qAbs(offset.x()) <= 1 && qAbs(offset.y()) <= 1;
        QCOMPARE(changed.count(), allowed ? 1 : 0);
        if (!allowed) {
            // Direct controller commands must not bypass the mouse's reach check.
            player->useCurrentToolAt(tileCenter(plot));
            QCOMPARE(handlerCalls, 0);
            QCOMPARE(changed.count(), 0);
            QCOMPARE(economy.state().inventoryCounts, assets.inventoryCounts);
            QCOMPARE(economy.state().seedCounts, assets.seedCounts);
            QCOMPARE(economy.fertilizerCount(), assets.fertilizerCount);
            QCOMPARE(economy.coins(), assets.coins);
            const auto after = map->worldSnapshot();
            QCOMPARE(after.cultivatedLayer, before.cultivatedLayer);
            QCOMPARE(after.wateredTiles, before.wateredTiles);
            QCOMPARE(after.crops.size(), before.crops.size());
            if (!after.crops.isEmpty()) {
                QCOMPARE(after.crops.first().growthDays, before.crops.first().growthDays);
                QCOMPARE(after.crops.first().fertilizerApplied, before.crops.first().fertilizerApplied);
                QCOMPARE(after.crops.first().fertilizerBonusPending, before.crops.first().fertilizerBonusPending);
            }
            QVERIFY(feedback.first().first().toString().contains(QStringLiteral("距离太远")));
        } else {
            QCOMPARE(handlerCalls, (action == 1 || action >= 3) ? 1 : 0);
            if (action == 1) QCOMPARE(economy.seedCount(CropType::Wheat), assets.seedCounts.value(1) - 1);
            if (action == 2) QVERIFY(map->worldSnapshot().wateredTiles.contains(plot));
            if (action == 3) QCOMPARE(economy.fertilizerCount(), assets.fertilizerCount - 1);
            if (action == 4) {
                QVERIFY(map->worldSnapshot().crops.isEmpty());
                QCOMPARE(economy.inventory().count(CropType::Wheat), 1);
            }
        }
    }

    void toolbarSelection() {
        QSignalSpy selected(bar, &ItemBar::itemSelected);
        QSignalSpy mapClicks(map, &MapWidget::mapClicked);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        const QPoint row = bar->pos() + QPoint(0, bar->height() - 12);
        const std::array expected{ToolType::Hoe, ToolType::WateringCan,
                                  ToolType::Wheat, ToolType::Potato, ToolType::Carrot,
                                  ToolType::Beet, ToolType::Fertilizer};
        QCOMPARE(ToolbarSlotCount, int(expected.size()));
        for (int i = 0; i < ToolbarSlotCount; ++i) {
            click(row + QPoint((2 * i + 1) * bar->width() / (2 * ToolbarSlotCount), 0));
            QCOMPARE(bar->selectedTool(), expected[i]);
            QCOMPARE(player->getCurrentTool().type, expected[i]);
            QVERIFY(!bar->accessibleDescription().isEmpty());
        }
        QCOMPARE(selected.count(), ToolbarSlotCount);
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
        QCOMPARE(bar->selectedTool(), ToolType::Hoe);
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

    void depletedSupplyClearsHeldItem() {
        EconomyService economy;
        bar->setStock(economy);
        bar->selectTool(ToolType::Wheat);
        QCOMPARE(player->getCurrentTool().type, ToolType::Wheat);
        const int count = economy.seedCount(CropType::Wheat);
        for (int i = 0; i < count; ++i)
            QVERIFY(economy.plantWithSeed(CropType::Wheat, [] { return true; }));
        bar->setStock(economy);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        QCOMPARE(player->getCurrentTool().type, ToolType::EmptyHand);
        bar->selectTool(ToolType::Wheat);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        QCOMPARE(economy.seedCount(CropType::Wheat), 0);
        bar->selectTool(ToolType::Hoe);
        QTest::keyClick(&view, Qt::Key_0);
        QCOMPARE(player->getCurrentTool().type, ToolType::EmptyHand);
    }

    void soilAutotileMasks_data() {
        QTest::addColumn<int>("mask");
        QTest::addColumn<bool>("wet");
        for (int mask = 0; mask < 16; ++mask) for (bool wet : {false,true})
            QTest::newRow(qPrintable(QStringLiteral("mask%1_%2").arg(mask).arg(wet))) << mask << wet;
    }

    void soilAutotileMasks() {
        QFETCH(int, mask); QFETCH(bool, wet);
        MapData data; QString error;
        QVERIFY(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &data, &error));
        WorldState world; world.initialize(data);
        auto state = world.snapshot();
        for (auto &row : state.cultivatedLayer) row.fill(0);
        state.crops.clear(); state.wateredTiles.clear();
        const QPoint center(5,5);
        state.cultivatedLayer[5][5] = 135;
        const QPoint directions[] = {{0,-1},{1,0},{0,1},{-1,0}};
        for (int i=0; i<4; ++i) if (mask & (1<<i)) {
            const QPoint at = center + directions[i];
            state.cultivatedLayer[at.y()][at.x()] = 135;
        }
        if (wet) state.wateredTiles.insert(center);
        QVERIFY(world.restore(state));
        auto tilesets = map->originalTilesets();
        for (auto &tileset : tilesets) if (tileset.firstGid == 123) {
            QPixmap texture(176,112); texture.fill(Qt::transparent);
            QPainter painter(&texture);
            for (int id=0; id<77; ++id)
                painter.fillRect((id%11)*16, (id/11)*16,16,16, QColor(10+id*3,40,60));
            painter.end(); tileset.texture = texture;
        }
        MapRenderLayer layer; layer.role = MapLayerRole::Cultivated;
        QPixmap output(16,16); output.fill(Qt::transparent);
        MapRenderer renderer;
        { QPainter painter(&output); renderer.paint(painter, output.rect(), center*16,
            data.mapWidth,data.mapHeight,16,16,tilesets,{layer},world); }
        // Independently enumerated expected 3x3 atlas cells for TL/TR/BL/BR.
        const int expected[16][4] = {
            {0,2,22,24},{11,13,22,24},{0,1,22,23},{11,12,22,23},
            {0,2,11,13},{11,13,11,13},{0,1,11,12},{11,12,11,12},
            {1,2,23,24},{12,13,23,24},{1,1,23,23},{12,12,23,23},
            {1,2,12,13},{12,13,12,13},{1,1,12,12},{12,12,12,12}};
        for (int q=0; q<4; ++q) {
            QPixmap reference(1,1); reference.fill(QColor(10+expected[mask][q]*3,40,60));
            if (wet) { QPainter tint(&reference); tint.fillRect(reference.rect(), QColor(64,92,112,105)); }
            QCOMPARE(output.toImage().pixelColor((q%2)*8+4,(q/2)*8+4),reference.toImage().pixelColor(0,0));
        }
        QCOMPARE(world.snapshot().cultivatedLayer, state.cultivatedLayer);
        QCOMPARE(world.snapshot().wateredTiles, state.wateredTiles);
        QCOMPARE(world.snapshot().lastCompletedDay, state.lastCompletedDay);
    }

    void soilAutotilePreviewAndRemoval() {
        MapData data; QString error;
        QVERIFY(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &data, &error));
        WorldState world; world.initialize(data);
        auto empty = world.snapshot();
        for (auto &row : empty.cultivatedLayer) row.fill(0);
        empty.crops.clear(); empty.wateredTiles.clear();
        MapRenderer renderer; MapRenderLayer layer; layer.role = MapLayerRole::Cultivated;
        const auto render = [&] {
            QPixmap image(80,80); image.fill(QColor("#a7bf70"));
            QPainter p(&image); renderer.paint(p,image.rect(),QPoint(3,3)*16,
                data.mapWidth,data.mapHeight,16,16,map->originalTilesets(),{layer},world);
            return image;
        };
        const QVector<QVector<QPoint>> shapes = {
            {{0,0}}, {{-1,0},{0,0},{1,0}}, {{0,-1},{0,0},{0,1}},
            {{0,0},{1,0},{0,1},{1,1}}, {{0,-1},{0,0},{1,0}},
            {{-1,0},{0,0},{1,0},{0,1}}, {{0,-1},{-1,0},{0,0},{1,0},{0,1}},
            {{-1,0},{0,0},{1,0},{0,1}}};
        const QStringList names{"Single","Horizontal","Vertical","Block","Corner","T junction","Cross","Mixed watering"};
        QPixmap sheet(960,520); sheet.fill(QColor("#26392f"));
        QPainter sheetPainter(&sheet);
        for (int i=0; i<shapes.size(); ++i) {
            auto state = empty;
            for (QPoint offset : shapes[i]) {
                const QPoint at = QPoint(5,5)+offset;
                state.cultivatedLayer[at.y()][at.x()] = 135;
                if (i==7 && offset.x()<=0) state.wateredTiles.insert(at);
            }
            QVERIFY(world.restore(state));
            const QPixmap image = render();
            const QRect tile((i%4)*240,(i/4)*260,240,240);
            sheetPainter.drawPixmap(tile,image);
            sheetPainter.setPen(Qt::white);
            sheetPainter.drawText(tile.translated(0,240).adjusted(0,0,0,-220),Qt::AlignCenter,names[i]);
        }
        sheetPainter.end(); QDir().mkpath(QStringLiteral("captures"));
        QVERIFY(sheet.save(QStringLiteral("captures/soil-autotile.png")));
        auto state = empty; state.cultivatedLayer[5][5] = 135;
        QVERIFY(world.restore(state)); const QImage single = render().toImage();
        state.cultivatedLayer[5][6] = 135;
        QVERIFY(world.restore(state)); QVERIFY(render().toImage() != single);
        state.cultivatedLayer[5][6] = 0;
        QVERIFY(world.restore(state)); QCOMPARE(render().toImage(),single);
        // Watering must preserve the rounded tile's transparent outer corner.
        QPixmap dry(16,16); dry.fill(Qt::transparent);
        { QPainter p(&dry); renderer.paint(p,dry.rect(),QPoint(5,5)*16,data.mapWidth,data.mapHeight,16,16,map->originalTilesets(),{layer},world); }
        state.wateredTiles.insert(QPoint(5,5)); QVERIFY(world.restore(state));
        QPixmap wet(16,16); wet.fill(Qt::transparent);
        { QPainter p(&wet); renderer.paint(p,wet.rect(),QPoint(5,5)*16,data.mapWidth,data.mapHeight,16,16,map->originalTilesets(),{layer},world); }
        for (int y=0; y<16; ++y) for (int x=0; x<16; ++x)
            QCOMPARE(wet.toImage().pixelColor(x,y).alpha(),dry.toImage().pixelColor(x,y).alpha());
        QVERIFY(dry.toImage() != wet.toImage());
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
        standAt(readyField);
        QVERIFY(map->isPlantable(readyField));
        click(tileCenter(readyField));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("这里可以直接播种"));

        const QPoint road(26, 13);
        standAt(QPoint(25, 13));
        click(tileCenter(road));
        QCOMPARE(feedback.count(), 1);
        QCOMPARE(feedback.takeFirst().first().toString(), QStringLiteral("道路不能耕种，请选草地"));
        QVERIFY(!map->isPlantable(road));
        QCOMPARE(changes.count(), 1);

        const QPoint besideChest(31, 21);
        standAt(besideChest);
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
        standAt(QPoint(25, 13));
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
            standAt(nextPlot);
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
        QTest::keyClick(&view, Qt::Key_E);
        QCOMPARE(inventory.count(), 2);
        QCOMPARE(interactions.count(), 0);
        QKeyEvent heldE(QEvent::KeyPress, Qt::Key_E, Qt::NoModifier, {}, true);
        QApplication::sendEvent(&view, &heldE);
        QCOMPARE(inventory.count(), 2);
        QTest::keyPress(&view, Qt::Key_D);
        QTRY_VERIFY(player->absolutePosition().x() > start.x());
        map->setInputBlocked(true);
        player->setInputBlocked(true);
        const auto paused = player->absolutePosition();
        QTest::keyClick(&view, Qt::Key_F);
        QTest::keyClick(&view, Qt::Key_E);
        QTest::keyClick(&view, Qt::Key_Tab);
        QTest::keyPress(&view, Qt::Key_S);
        click(tileCenter(plot));
        scroll(tileCenter(plot), -120);
        QCOMPARE(clicks.count(), 0);
        QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
        QCOMPARE(interactions.count(), 0);
        QCOMPARE(inventory.count(), 2);
        // Selecting in a panel still equips the tool during the lock.
        bar->selectTool(ToolType::Carrot);
        map->setInputBlocked(false);
        player->setInputBlocked(false);
        QTest::qWait(220);
        QCOMPARE(player->absolutePosition(), paused);
        QCOMPARE(player->getCurrentTool().type, ToolType::Carrot);
        QTest::keyRelease(&view, Qt::Key_S);
        QTest::keyRelease(&view, Qt::Key_D);
        QTest::keyClick(&view, Qt::Key_F);
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
        standAt(QPoint(26, 13)); // Align with the original doorway approach.
        const QRect bedside(25, 8, 2, 2);
        QTest::keyPress(&view, Qt::Key_W);
        QTRY_VERIFY(bedside.contains(player->currentTilePosition()));
        QTest::keyRelease(&view, Qt::Key_W);
        QSignalSpy interactions(player, &ImageMovementWidget::interactionRequested);
        QTest::keyClick(&view, Qt::Key_F);
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
