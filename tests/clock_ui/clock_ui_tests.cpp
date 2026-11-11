#include "mainwindow.h"
#include "scenetransition.h"

#include <QAudioDevice>
#include <QAudioOutput>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsProxyWidget>
#include <QListWidget>
#include <QMediaDevices>
#include <QPointer>
#include <QSignalSpy>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <QUuid>
#include <QWheelEvent>
#include <cmath>
#include <memory>

namespace {

constexpr int MinutesPerDay = 24 * 60;
const QPoint FarmTile(21, 9);
// Character geometry is 48 x 48. Place its centre on the walkable bed marker.
const QPoint BesideBed(25 * 16 + 8 - 24, 9 * 16 + 8 - 24);
const QPoint OutsideHouse(400, 192);

} // namespace

class ClockUiTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();
    void eveningSleepUsesOneTransition_data();
    void eveningSleepUsesOneTransition();
    void ordinaryMidnightThenDawnSleep();
    void winterMidnightStartsNewYear();
    void closingDuringFadeRestoresCommittedMorning();
    void seasonChangesWithBackpackOpen();
    void onlySuccessfulActionsPlaySounds();
    void firstShowFitsViewport_data();
    void firstShowFitsViewport();
    void loadCatchesUpMissedMidnights();
    void sleepRefused_data();
    void sleepRefused();

private:
    bool writeFarm(int day, int minuteOfDay, QPoint position = BesideBed);
    bool openGame(QSize initialSize = {});
    QString hudText() const;
    QColor grassColour() const;
    void verifySaved(int day, int hour, int growth, bool watered);
    void pressInteract();
    void clickCanvas(QWidget *widget, const QPoint &point);
    void clickTile(const QPoint &tile);

    QString m_applicationId;
    MapData m_terrain;
    std::unique_ptr<MainWindow> m_window;
    QWidget *m_canvas = nullptr;
    ImageMovementWidget *m_character = nullptr;
    MapWidget *m_map = nullptr;
    SceneTransition *m_fade = nullptr;
    QLabel *m_date = nullptr;
    QLabel *m_time = nullptr;
};

void ClockUiTests::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
    m_applicationId = QStringLiteral("clock-ui-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QCoreApplication::setOrganizationName(QStringLiteral("CloverCogTests"));
    QCoreApplication::setApplicationName(m_applicationId);
    QCOMPARE(QFileInfo(SaveManager::defaultSavePath()).dir().dirName(), m_applicationId);
    QCOMPARE(QFileInfo(SaveManager::legacySavePath()).dir(), QFileInfo(SaveManager::defaultSavePath()).dir());
    QString error;
    QVERIFY2(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &m_terrain, &error),
             qPrintable(error));
}

void ClockUiTests::cleanup() {
    m_window.reset();
    QDir directory = QFileInfo(SaveManager::defaultSavePath()).dir();
    if (directory.dirName() == m_applicationId && directory.exists())
        QVERIFY(directory.removeRecursively());
}

void ClockUiTests::cleanupTestCase() {
    QStandardPaths::setTestModeEnabled(false);
}

bool ClockUiTests::writeFarm(int day, int minuteOfDay, QPoint position) {
    WorldState world;
    world.initialize(m_terrain);
    if (day > 0)
        world.advanceDay(day - 1, [] { return 99; });
    if (!world.cultivate(FarmTile) || !world.plantSeed(FarmTile, CropType::Wheat, day)
        || !world.waterTile(FarmTile) || !world.fertilize(FarmTile).success)
        return false;

    GameSaveData data;
    data.gameMinutes = qint64(day) * MinutesPerDay + minuteOfDay;
    data.lastProcessedDay = day;
    data.playerPosition = position;
    data.world = world.snapshot();
    data.economy = EconomyService().state();
    return SaveManager::save(SaveManager::defaultSavePath(), data);
}

bool ClockUiTests::openGame(QSize initialSize) {
    m_window = std::make_unique<MainWindow>();
    if (auto *music = m_window->findChild<QAudioOutput *>())
        music->setMuted(true);
    if (initialSize.isValid())
        m_window->resize(initialSize);
    m_window->show();
    m_window->activateWindow();
    if (!QTest::qWaitForWindowExposed(m_window.get()))
        return false;
    auto *view = m_window->findChild<QGraphicsView *>();
    if (!view)
        return false;
    m_canvas = nullptr;
    for (QGraphicsItem *item : view->scene()->items()) {
        if (auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
            m_canvas = proxy->widget();
            if (m_canvas)
                break;
        }
    }
    if (!m_canvas)
        return false;
    m_character = m_canvas->findChild<ImageMovementWidget *>();
    m_map = m_canvas->findChild<MapWidget *>();
    m_fade = m_canvas->findChild<SceneTransition *>();
    m_date = m_canvas->findChild<QLabel *>("gameDate");
    m_time = m_canvas->findChild<QLabel *>("gameTime");
    return m_character && m_map && m_fade && m_date && m_time;
}

QString ClockUiTests::hudText() const {
    QStringList visible;
    for (const auto *label : m_canvas->findChildren<QLabel *>()) {
        if (label->isVisibleTo(m_canvas))
            visible.append(label->text());
    }
    return visible.join(QLatin1Char(' '));
}

QColor ClockUiTests::grassColour() const {
    // Sample an empty grass tile beside the house. Render the actual map
    // without child widgets, so character, light and HUD cannot affect it.
    const QPoint pixel(20 * 16 + 8 - m_map->offsetX(), 9 * 16 + 8 - m_map->offsetY());
    if (!m_map->rect().contains(pixel))
        return {};
    QImage terrain(m_map->size(), QImage::Format_ARGB32_Premultiplied);
    terrain.fill(Qt::transparent);
    m_map->render(&terrain, QPoint(), QRegion(), QWidget::DrawWindowBackground);
    return terrain.pixelColor(pixel);
}

void ClockUiTests::pressInteract() {
    QTest::keyClick(m_character, Qt::Key_E);
}

void ClockUiTests::clickCanvas(QWidget *widget, const QPoint &point) {
    auto *view = m_window->findChild<QGraphicsView *>();
    const auto scenePoint = m_canvas->graphicsProxyWidget()->mapToScene(widget->mapTo(m_canvas, point));
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, view->mapFromScene(scenePoint));
}

void ClockUiTests::clickTile(const QPoint &tile) {
    const QPoint screen(tile.x() * m_map->tileWidth() + m_map->tileWidth() / 2 - m_map->offsetX(),
                        tile.y() * m_map->tileHeight() + m_map->tileHeight() / 2 - m_map->offsetY());
    clickCanvas(m_map, screen);
}

void ClockUiTests::verifySaved(int day, int hour, int growth, bool watered) {
    GameSaveData saved;
    QString error;
    QVERIFY2(SaveManager::load(SaveManager::defaultSavePath(), &saved, &error), qPrintable(error));
    QCOMPARE(saved.gameMinutes / MinutesPerDay, day);
    QCOMPARE((saved.gameMinutes % MinutesPerDay) / 60, hour);
    QCOMPARE(saved.lastProcessedDay, day);
    QCOMPARE(saved.world.lastCompletedDay, day - 1);
    QCOMPARE(saved.world.crops.size(), 1);
    const SavedCropState &crop = saved.world.crops.front();
    QCOMPARE(crop.tilePos, FarmTile);
    QCOMPARE(crop.growthDays, growth);
    QVERIFY(crop.fertilizerApplied);
    QCOMPARE(crop.fertilizerBonusPending, growth == 0);
    QCOMPARE(saved.world.wateredTiles.contains(FarmTile), watered);
}

void ClockUiTests::eveningSleepUsesOneTransition_data() {
    QTest::addColumn<int>("day");
    QTest::addColumn<QString>("morning");
    QTest::newRow("ordinary-evening") << 1 << QStringLiteral("第 1 年 · 春季 3 日");
    QTest::newRow("last-evening-of-spring") << 6 << QStringLiteral("第 1 年 · 夏季 1 日");
}

void ClockUiTests::eveningSleepUsesOneTransition() {
    QFETCH(int, day);
    QFETCH(QString, morning);
    const QString evening = QStringLiteral("第 1 年 · 春季 %1 日").arg(day + 1);
    QVERIFY(writeFarm(day, 20 * 60));
    QVERIFY(openGame());
    QCOMPARE(m_character->currentTilePosition(), QPoint(25, 9));
    QCOMPARE(m_date->text(), evening);
    QSignalSpy black(m_fade, &SceneTransition::blackReached);
    QSignalSpy finished(m_fade, &SceneTransition::finished);
    QSignalSpy clicks(m_map, &MapWidget::mapClicked);
    pressInteract();
    QVERIFY(m_fade->isActive());
    QCOMPARE(m_window->currentGameDay(), day + 1);
    verifySaved(day + 1, 6, 2, false);
    QCOMPARE(m_date->text(), evening);
    QCOMPARE(black.count(), 0);

    auto *view = m_window->findChild<QGraphicsView *>();
    auto *bar = m_canvas->findChild<ItemBar *>();
    QVERIFY(bar && view);
    const auto scrollOverFarm = [&] {
        const QPoint canvasPoint = m_map->mapTo(m_canvas, QPoint(70, 100));
        const QPoint pos = view->mapFromScene(m_canvas->graphicsProxyWidget()->mapToScene(canvasPoint));
        QWheelEvent wheel(pos, view->viewport()->mapToGlobal(pos), {}, QPoint(0, -120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(view->viewport(), &wheel);
    };
    scrollOverFarm();
    QCOMPARE(bar->selectedTool(), ToolType::EmptyHand);
    pressInteract();
    QTest::mouseClick(m_map, Qt::LeftButton, Qt::NoModifier, QPoint(70, 100));
    const QPoint before = m_character->absolutePosition();
    QTest::keyPress(m_character, Qt::Key_D);
    QTest::qWait(125);
    QTest::keyRelease(m_character, Qt::Key_D);
    QCOMPARE(m_character->absolutePosition(), before);
    QCOMPARE(clicks.count(), 0);

    QTRY_COMPARE_WITH_TIMEOUT(black.count(), 1, 2000);
    QCOMPARE(m_date->text(), morning);
    QVERIFY(m_time->text().contains(QStringLiteral("06:")));
    pressInteract();
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2000);
    QVERIFY(!m_fade->isActive());
    QCOMPARE(black.count(), 1);
    QTest::mouseClick(m_map, Qt::LeftButton, Qt::NoModifier, QPoint(70, 100));
    QCOMPARE(clicks.count(), 1);
    scrollOverFarm();
    QCOMPARE(bar->selectedTool(), ToolType::Axe);
    QCOMPARE(m_character->getCurrentTool().type, ToolType::Axe);
    verifySaved(day + 1, 6, 2, false);
}

void ClockUiTests::ordinaryMidnightThenDawnSleep() {
    QVERIFY(writeFarm(2, 23 * 60 + 59));
    QVERIFY(openGame());
    QSignalSpy black(m_fade, &SceneTransition::blackReached);
    QSignalSpy finished(m_fade, &SceneTransition::finished);
    QTRY_COMPARE_WITH_TIMEOUT(m_map->worldSnapshot().lastCompletedDay, 2, 4000);
    verifySaved(3, 0, 2, false);
    QVERIFY(!m_fade->isActive());
    QCOMPARE(black.count(), 0);
    // Water on the new day before sleeping. A second settlement would
    // consume this water and mature the wheat prematurely.
    QVERIFY(m_map->waterTile(FarmTile));
    pressInteract();
    QVERIFY(m_fade->isActive());
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2000);
    QCOMPARE(black.count(), 1);
    verifySaved(3, 6, 2, true);
}

void ClockUiTests::winterMidnightStartsNewYear() {
    QVERIFY(writeFarm(27, 23 * 60 + 59));
    QVERIFY(openGame());
    QCOMPARE(m_date->text(), QStringLiteral("第 1 年 · 冬季 7 日"));
    const QColor snowyGrass = grassColour();
    QVERIFY(snowyGrass.isValid());
    QSignalSpy black(m_fade, &SceneTransition::blackReached);
    QSignalSpy finished(m_fade, &SceneTransition::finished);
    QTRY_COMPARE_WITH_TIMEOUT(black.count(), 1, 4000);
    QCOMPARE(m_date->text(), QStringLiteral("第 2 年 · 春季 1 日"));
    const QColor springGrass = grassColour();
    QVERIFY(springGrass.isValid());
    QVERIFY(springGrass != snowyGrass);
    QVERIFY(m_time->text().contains(QStringLiteral("00:")));
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2000);
    verifySaved(28, 0, 2, false);
}

void ClockUiTests::closingDuringFadeRestoresCommittedMorning() {
    QVERIFY(writeFarm(6, 22 * 60));
    QVERIFY(openGame());
    pressInteract();
    QVERIFY(m_fade->isActive());
    QVERIFY(m_window->close());
    m_window.reset();
    verifySaved(7, 6, 2, false);

    QVERIFY(openGame());
    QVERIFY(!m_fade->isActive());
    QCOMPARE(m_date->text(), QStringLiteral("第 1 年 · 夏季 1 日"));
    QVERIFY(m_time->text().contains(QStringLiteral("06:")));
    QCOMPARE(m_map->worldSnapshot().lastCompletedDay, 6);
    const auto restoredCrops = m_map->worldSnapshot().crops;
    QCOMPARE(restoredCrops.size(), 1);
    QCOMPARE(restoredCrops.front().growthDays, 2);
    QVERIFY(m_window->close());
    verifySaved(7, 6, 2, false);
}

void ClockUiTests::seasonChangesWithBackpackOpen() {
    QVERIFY(writeFarm(6, 23 * 60 + 58));
    GameSaveData sample;
    const QString path = SaveManager::defaultSavePath();
    QVERIFY(SaveManager::load(path, &sample));
    sample.economy.inventoryCounts[static_cast<int>(CropType::Carrot)] = 3;
    sample.economy.fertilizerCount = 2;
    QVERIFY(SaveManager::save(path, sample));
    QVERIFY(openGame());
    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QPointer<QDialog> bag = m_window->findChild<QDialog *>("inventoryDialog");
    QVERIFY(bag && bag->isVisible());
    auto *tabs = bag->findChild<QTabWidget *>("bagCategories");
    auto *seeds = bag->findChild<QListWidget *>("bagSeeds");
    auto *date = bag->findChild<QLabel *>("bagDate");
    auto *summary = bag->findChild<QLabel *>("panelSummary");
    QVERIFY(tabs && seeds && date && summary);
    QTest::mouseClick(tabs->tabBar(), Qt::LeftButton, {}, tabs->tabBar()->tabRect(1).center());
    QCOMPARE(tabs->currentWidget(), seeds);
    QTRY_VERIFY(QApplication::focusWidget() && QApplication::focusWidget()->window() == bag);
    QPointer<QWidget> previousFocus = QApplication::focusWidget();
    QVERIFY(date->text().contains(QStringLiteral("春季")));
    QSignalSpy finished(m_fade, &SceneTransition::finished);

    QTRY_VERIFY_WITH_TIMEOUT(m_fade->isActive(), 4500);
    QVERIFY(!bag->isEnabled());
    QVERIFY(m_map->isInputBlocked());
    const QRect seedRow = seeds->visualItemRect(seeds->item(0));
    QVERIFY(!seedRow.isEmpty());
    QTest::mouseClick(seeds->viewport(), Qt::LeftButton, {}, seedRow.center());
    QVERIFY(bag && bag->isVisible());
    for (const auto key : {Qt::Key_Tab, Qt::Key_Escape}) {
        QTest::keyClick(bag->windowHandle(), key);
        QVERIFY(bag && bag->isVisible());
    }
    QCOMPARE(m_character->getCurrentTool().type, ToolType::EmptyHand);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2000);
    QVERIFY(bag && bag->isEnabled() && bag->isVisible());
    QTRY_COMPARE(QApplication::focusWidget(), previousFocus.data());
    QVERIFY(m_map->isInputBlocked());
    QCOMPARE(tabs->currentWidget(), seeds);
    QVERIFY(date->text().contains(QStringLiteral("夏季")));
    QCOMPARE(summary->text().simplified(), QStringLiteral("农产品 3 / 12"));

    auto *fertilizer = bag->findChild<QListWidget *>("bagFertilizer");
    QVERIFY(fertilizer && fertilizer->count() > 0);
    QCOMPARE(fertilizer->item(0)->text().simplified(), QStringLiteral("基础肥料 × 2"));
    verifySaved(7, 0, 2, false);
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Escape);
    QTRY_VERIFY(!bag || !bag->isVisible());
    QTRY_VERIFY(!m_map->isInputBlocked());
}

void ClockUiTests::onlySuccessfulActionsPlaySounds() {
    if (QMediaDevices::defaultAudioOutput().isNull())
        QSKIP("Playback requires an audio output device");
    QVERIFY(writeFarm(2, 12 * 60));
    GameSaveData sample;
    const QString path = SaveManager::defaultSavePath();
    QVERIFY(SaveManager::load(path, &sample));
    sample.world.wateredTiles.clear();
    sample.economy.inventoryCounts[static_cast<int>(CropType::Wheat)] =
        sample.economy.inventoryCapacity - 1;
    const QPoint ripe = FarmTile - QPoint(1, 0);
    const QPoint extra = FarmTile - QPoint(2, 0);
    for (const QPoint &tile : {ripe, extra}) {
        sample.world.cultivatedLayer[tile.y()][tile.x()] = GameConstants::CultivatedTileGid;
        sample.world.crops.append({tile, CropType::Wheat, 0,
                                   CropCatalog::definition(CropType::Wheat).growthDays});
    }
    QVERIFY(SaveManager::save(path, sample));
    QVERIFY(openGame());
    auto *water = m_window->findChild<QSoundEffect *>("wateringSound");
    auto *harvest = m_window->findChild<QSoundEffect *>("harvestSound");
    auto *bar = m_canvas->findChild<ItemBar *>();
    QVERIFY(water && harvest && bar);
    QTRY_COMPARE(water->status(), QSoundEffect::Ready);
    QTRY_COMPARE(harvest->status(), QSoundEffect::Ready);
    QCOMPARE(water->source(), QUrl(QStringLiteral("qrc:/sounds/water.wav")));
    QCOMPARE(harvest->source(), QUrl(QStringLiteral("qrc:/sounds/harvest.wav")));
    QSignalSpy waterPlayback(water, &QSoundEffect::playingChanged);
    QSignalSpy harvestPlayback(harvest, &QSoundEffect::playingChanged);
    QSignalSpy watered(m_map, &MapWidget::tileWatered);
    QSignalSpy collected(m_map, &MapWidget::cropHarvested);
    QSignalSpy blocked(m_map, &MapWidget::harvestBlocked);

    const int slotWidth = bar->sizeHint().width() / ToolCount;
    const int firstSlot = (bar->width() - ToolCount * slotWidth) / 2;
    const int waterSlot = static_cast<int>(ToolType::WateringCan);
    clickCanvas(bar, QPoint(firstSlot + waterSlot * slotWidth + slotWidth / 2,
                            bar->height() - slotWidth / 2));
    QCOMPARE(m_character->getCurrentTool().type, ToolType::WateringCan);
    clickTile(FarmTile);
    QCOMPARE(watered.count(), 1);
    QTRY_COMPARE(waterPlayback.count(), 2); // started and finished
    clickTile(FarmTile);
    QCOMPARE(watered.count(), 1);
    QCOMPARE(waterPlayback.count(), 2);
    QCOMPARE(harvestPlayback.count(), 0);

    clickTile(ripe);
    QCOMPARE(collected.count(), 1);
    QTRY_COMPARE(harvestPlayback.count(), 2);
    clickTile(extra);
    QCOMPARE(blocked.count(), 1);
    QCOMPARE(collected.count(), 1);
    QCOMPARE(harvestPlayback.count(), 2);
    QCOMPARE(waterPlayback.count(), 2);
    GameSaveData saved;
    QVERIFY(SaveManager::load(path, &saved));
    QCOMPARE(saved.economy.inventoryCounts.value(static_cast<int>(CropType::Wheat)),
             sample.economy.inventoryCapacity);
    QCOMPARE(saved.world.crops.size(), 2);
}

void ClockUiTests::firstShowFitsViewport_data() {
    QTest::addColumn<QSize>("size");
    QTest::newRow("smaller-than-canvas") << QSize(420, 315);
    QTest::newRow("retina-half-step") << QSize(1040, 640);
    QTest::newRow("room-for-two-pixels") << QSize(1180, 875);
}

void ClockUiTests::firstShowFitsViewport() {
    QFETCH(QSize, size);
    QVERIFY(openGame(size));
    auto *view = m_window->findChild<QGraphicsView *>();
    auto *bar = m_canvas->findChild<ItemBar *>();
    auto *header = m_canvas->findChild<QWidget *>("farmHeader");
    QVERIFY(view && bar && header);
    const QSize viewport = view->viewport()->size();
    const qreal available = qMin(qreal(viewport.width()) / m_canvas->width(),
                                 qreal(viewport.height()) / m_canvas->height());
    const qreal dpr = view->viewport()->devicePixelRatioF();
    const qreal expected = available >= 1 ? std::floor(available * dpr) / dpr : available;
    QCOMPARE(view->transform().m11(), expected);
    QCOMPARE(view->transform().m22(), expected);
    const QRect canvasOnScreen = view->mapFromScene(view->sceneRect()).boundingRect();
    QVERIFY(view->viewport()->rect().adjusted(-1, -1, 1, 1).contains(canvasOnScreen));
    QVERIFY((view->viewport()->rect().center() - canvasOnScreen.center()).manhattanLength() <= 2);
    QVERIFY(m_canvas->rect().contains(header->geometry()));
    QVERIFY(m_canvas->rect().contains(bar->geometry()));
    QVERIFY(header->geometry().bottom() < m_map->geometry().top());
    QVERIFY(bar->geometry().top() > m_map->geometry().bottom());
}

void ClockUiTests::loadCatchesUpMissedMidnights() {
    QVERIFY(writeFarm(2, 18 * 60));
    GameSaveData late;
    const QString path = SaveManager::defaultSavePath();
    QVERIFY(SaveManager::load(path, &late));
    late.gameMinutes += 3 * MinutesPerDay;
    QVERIFY(SaveManager::save(path, late));

    QVERIFY(openGame());
    QTRY_COMPARE_WITH_TIMEOUT(m_map->worldSnapshot().lastCompletedDay, 4, 2000);
    QVERIFY(!m_fade->isActive());
    verifySaved(5, 18, 2, false);
}

void ClockUiTests::sleepRefused_data() {
    QTest::addColumn<QPoint>("position");
    QTest::addColumn<int>("hour");
    QTest::newRow("daytime-at-bed") << BesideBed << 13;
    QTest::newRow("night-away-from-bed") << OutsideHouse << 21;
}

void ClockUiTests::sleepRefused() {
    QFETCH(QPoint, position);
    QFETCH(int, hour);
    QVERIFY(writeFarm(2, hour * 60, position));
    QVERIFY(openGame());
    pressInteract();
    QVERIFY(!m_fade->isActive());
    QCOMPARE(m_window->currentGameHour(), hour);
    if (position == BesideBed)
        QVERIFY(hudText().contains(QStringLiteral("20:00")));
    QVERIFY(m_window->close());
    verifySaved(2, hour, 0, true);
}

QTEST_MAIN(ClockUiTests)
#include "clock_ui_tests.moc"
