#include <QtTest>
#include <QAudioOutput>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <memory>

#include "mainwindow.h"
#include "mapdata.h"
#include "savemanager.h"

namespace {
QByteArray readBytes(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool writeBytes(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.flush();
}

void showGame(MainWindow &game) {
    if (auto *audio = game.findChild<QAudioOutput *>()) {
        audio->setMuted(true);
    }
    game.show();
    game.activateWindow();
    QCoreApplication::processEvents();
}

QDialog *panel(const QString &name) {
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (widget->isVisible() && widget->objectName() == name) {
            return qobject_cast<QDialog *>(widget);
        }
    }
    return nullptr;
}

MapWidget *gameMap(MainWindow &game) {
    auto *view = game.findChild<QGraphicsView *>();
    if (!view) {
        return nullptr;
    }
    for (QGraphicsItem *item : view->scene()->items()) {
        auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item);
        if (proxy && proxy->widget()) {
            if (auto *map = proxy->widget()->findChild<MapWidget *>()) {
                return map;
            }
        }
    }
    return nullptr;
}
}

class GameplayPanelTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();
    void cleanup();
    void unsupportedLegacyFilesRemainAlongsideNewGame();
    void migrateVersionTwo_data();
    void migrateVersionTwo();
    void invalidPlayerPositionRecoversFromBackup();
    void damagedCurrentFilesRemainUntouched();
    void buySelectAndUseFertilizer();
    void sellAtMarket();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    MapData m_mapData;
    GameSaveData m_fixture;
    QPoint m_field{31, 18};

    QString path() const { return SaveManager::defaultSavePath(); }
    QString oldPath() const { return SaveManager::legacySavePath(); }
    QPoint stationPosition(const QPoint &tile) const;
};

QPoint GameplayPanelTests::stationPosition(const QPoint &tile) const {
    // Saved positions are measured at the top left of a 48 x 48 player.
    return QPoint(tile.x() * m_mapData.tileWidth + m_mapData.tileWidth / 2 - 24,
                  tile.y() * m_mapData.tileHeight + m_mapData.tileHeight / 2 - 24);
}

void GameplayPanelTests::initTestCase() {
    QString error;
    QVERIFY2(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &m_mapData, &error),
             qPrintable(error));
    WorldState world;
    world.initialize(m_mapData);
    m_fixture.world = world.snapshot();
    m_fixture.world.cultivatedLayer[m_field.y()][m_field.x()] = GameConstants::CultivatedTileGid;
    m_fixture.world.crops.append({m_field, CropType::Wheat, 0, 0, false, false});
    m_fixture.gameMinutes = 8 * 60;
    m_fixture.playerPosition = stationPosition(QPoint(33, 18));
    m_fixture.economy = EconomyService().state();
    m_fixture.economy.coins = 43;
}

void GameplayPanelTests::init() {
    QCoreApplication::setOrganizationName(QString());
    QCoreApplication::setApplicationName(QStringLiteral("CloverCog-location-test"));
    const QDir parent = QFileInfo(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).dir();
    QVERIFY(parent.mkpath(QStringLiteral(".")));
    m_directory = std::make_unique<QTemporaryDir>(parent.filePath(QStringLiteral("CloverCog-test-XXXXXX")));
    QVERIFY(m_directory->isValid());
    QCoreApplication::setApplicationName(QFileInfo(m_directory->path()).fileName());
    // Refuse to create a game until both save paths resolve to this unique directory.
    QCOMPARE(QFileInfo(path()).absolutePath(), m_directory->path());
    QCOMPARE(QFileInfo(oldPath()).absolutePath(), m_directory->path());
}

void GameplayPanelTests::cleanup() {
    m_directory.reset();
}

void GameplayPanelTests::unsupportedLegacyFilesRemainAlongsideNewGame() {
    const QByteArray primary = R"({"schemaVersion":1,"coins":19})";
    const QByteArray backup = R"({"schemaVersion":1,"coins":5})";
    QVERIFY(writeBytes(oldPath(), primary));
    QVERIFY(writeBytes(oldPath() + ".bak", backup));
    MainWindow game;
    showGame(game);
    QVERIFY(game.close());
    GameSaveData fresh;
    QVERIFY(SaveManager::load(path(), &fresh));
    QCOMPARE(fresh.economy.coins, 0);
    QCOMPARE(fresh.economy.seedCounts.value(static_cast<int>(CropType::Wheat)), 6);
    QCOMPARE(readBytes(oldPath()), primary);
    QCOMPARE(readBytes(oldPath() + ".bak"), backup);
    QVERIFY(SaveManager::load(path() + ".bak", &fresh));
}

void GameplayPanelTests::migrateVersionTwo_data() {
    QTest::addColumn<bool>("useBackup");
    QTest::newRow("valid-primary") << false;
    QTest::newRow("damaged-primary-valid-backup") << true;
}

void GameplayPanelTests::migrateVersionTwo() {
    QFETCH(bool, useBackup);
    GameSaveData previous = m_fixture;
    previous.world.crops[0].fertilizerApplied = true;
    previous.world.crops[0].fertilizerBonusPending = true;
    const QString sample = m_directory->filePath("sample.json");
    QVERIFY(SaveManager::save(sample, previous));
    QJsonObject root = QJsonDocument::fromJson(readBytes(sample)).object();
    root["schemaVersion"] = 2;
    QJsonObject economy = root["economy"].toObject();
    economy.remove("fertilizerCount");
    root["economy"] = economy;
    const QByteArray legacy = QJsonDocument(root).toJson();
    const QByteArray primary = useBackup ? QByteArray("{torn") : legacy;
    QVERIFY(writeBytes(oldPath(), primary));
    if (useBackup) {
        QVERIFY(writeBytes(oldPath() + ".bak", legacy));
    }

    MainWindow game;
    showGame(game);
    QVERIFY(game.close());
    GameSaveData fresh;
    QVERIFY(SaveManager::load(path(), &fresh));
    QCOMPARE(fresh.economy.coins, 43);
    QCOMPARE(fresh.economy.fertilizerCount, 0);
    QCOMPARE(fresh.playerPosition, previous.playerPosition);
    QCOMPARE(fresh.world.crops.size(), 1);
    QVERIFY(fresh.world.crops.first().fertilizerApplied);
    QVERIFY(fresh.world.crops.first().fertilizerBonusPending);
    QCOMPARE(readBytes(oldPath()), primary);
    if (useBackup) {
        QCOMPARE(readBytes(oldPath() + ".bak"), legacy);
    }
}

void GameplayPanelTests::invalidPlayerPositionRecoversFromBackup() {
    QVERIFY(SaveManager::save(path(), m_fixture));
    const QByteArray backup = readBytes(path() + ".bak");
    QJsonObject root = QJsonDocument::fromJson(readBytes(path())).object();
    root["playerPosition"] = QJsonObject{{"x", 1000000}, {"y", 0}};
    QJsonObject economy = root["economy"].toObject();
    economy["coins"] = 999;
    root["economy"] = economy;
    QVERIFY(writeBytes(path(), QJsonDocument(root).toJson()));

    MainWindow game;
    showGame(game);
    QVERIFY(game.close());
    QCOMPARE(readBytes(path() + ".bak"), backup);
    GameSaveData recovered;
    QVERIFY(SaveManager::load(path(), &recovered));
    QCOMPARE(recovered.economy.coins, m_fixture.economy.coins);
    QCOMPARE(recovered.playerPosition, m_fixture.playerPosition);
}

void GameplayPanelTests::damagedCurrentFilesRemainUntouched() {
    const QByteArray primary = "{partial";
    const QByteArray backup = R"({"schemaVersion":3})";
    QVERIFY(writeBytes(path(), primary));
    QVERIFY(writeBytes(path() + ".bak", backup));
    MainWindow game;
    showGame(game);
    QVERIFY(game.close());
    QCOMPARE(readBytes(path()), primary);
    QCOMPARE(readBytes(path() + ".bak"), backup);
}

void GameplayPanelTests::buySelectAndUseFertilizer() {
    QVERIFY(SaveManager::save(path(), m_fixture));
    MainWindow game;
    showGame(game);
    auto *map = gameMap(game);
    QVERIFY(map);
    auto *player = map->findChild<ImageMovementWidget *>();
    auto *bar = map->parentWidget()->findChild<ItemBar *>();
    QVERIFY(player && bar);
    QCOMPARE(player->currentTilePosition(), QPoint(33, 18));
    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QTRY_VERIFY(panel("inventoryDialog"));
    QTRY_VERIFY(QApplication::focusWidget() &&
                QApplication::focusWidget()->window() == panel("inventoryDialog"));
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QTRY_VERIFY(!panel("inventoryDialog"));

    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_E);
    QTRY_VERIFY(panel("shopDialog"));
    auto *buy = panel("shopDialog")->findChild<QPushButton *>("buy_fertilizer_1");
    QVERIFY(buy && buy->isEnabled());
    QTest::mouseClick(buy, Qt::LeftButton);
    QTest::mouseClick(buy, Qt::LeftButton);
    GameSaveData purchased;
    QVERIFY(SaveManager::load(path(), &purchased));
    QCOMPARE(purchased.economy.fertilizerCount, 2);
    QCOMPARE(purchased.economy.coins, 41);
    QTRY_VERIFY(QApplication::focusWidget() &&
                QApplication::focusWidget()->window() == panel("shopDialog"));
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Escape);
    QTRY_VERIFY(!panel("shopDialog"));

    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QTRY_VERIFY(panel("inventoryDialog"));
    auto *bag = panel("inventoryDialog");
    auto *categories = bag->findChild<QTabWidget *>("bagCategories");
    QVERIFY(categories);
    QTabBar *tabBar = categories->tabBar();
    QTest::mouseClick(tabBar, Qt::LeftButton, {}, tabBar->tabRect(2).center());
    auto *fertilizer = bag->findChild<QListWidget *>("bagFertilizer");
    QVERIFY(fertilizer && fertilizer->isVisible() && fertilizer->count() > 0);
    const QRect entry = fertilizer->visualItemRect(fertilizer->item(0));
    QVERIFY(!entry.isEmpty());
    QTest::mouseClick(fertilizer->viewport(), Qt::LeftButton, {}, entry.center());
    QTRY_VERIFY(!panel("inventoryDialog"));
    QTRY_VERIFY(!map->isInputBlocked() && QApplication::focusWidget());
    QCOMPARE(player->getCurrentTool().type, ToolType::Fertilizer);
    QCOMPARE(bar->selectedTool(), ToolType::Fertilizer);

    const QPoint mapPoint = QPoint(m_field.x() * map->tileWidth() + map->tileWidth() / 2,
                                   m_field.y() * map->tileHeight() + map->tileHeight() / 2) -
                            QPoint(map->offsetX(), map->offsetY());
    auto *view = game.findChild<QGraphicsView *>();
    QWidget *canvas = map->window();
    QVERIFY(canvas && canvas->graphicsProxyWidget());
    const QPoint viewportPoint = view->mapFromScene(canvas->graphicsProxyWidget()->mapToScene(
        map->mapTo(canvas, mapPoint)));
    QSignalSpy changes(map, &MapWidget::worldChanged);
    QSignalSpy feedback(map, &MapWidget::farmingFeedback);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, viewportPoint);
    QCOMPARE(changes.count(), 1);
    GameSaveData applied;
    QVERIFY(SaveManager::load(path(), &applied));
    QCOMPARE(applied.economy.fertilizerCount, 1);
    QCOMPARE(applied.world.crops.size(), 1);
    QVERIFY(applied.world.crops.first().fertilizerApplied);
    QVERIFY(applied.world.crops.first().fertilizerBonusPending);
    feedback.clear();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, viewportPoint);
    QCOMPARE(feedback.count(), 1);
    QCOMPARE(changes.count(), 1);
    QVERIFY(SaveManager::load(path(), &applied));
    QCOMPARE(applied.economy.fertilizerCount, 1);
    QVERIFY(applied.world.crops.first().fertilizerBonusPending);
}

void GameplayPanelTests::sellAtMarket() {
    GameSaveData fixture = m_fixture;
    fixture.playerPosition = stationPosition(QPoint(28, 18));
    fixture.economy.inventoryCounts[static_cast<int>(CropType::Wheat)] = 6;
    QVERIFY(SaveManager::save(path(), fixture));
    MainWindow game;
    showGame(game);
    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_E);
    QTRY_VERIFY(panel("marketDialog"));
    auto *sell = panel("marketDialog")->findChild<QPushButton *>("sellAll");
    QVERIFY(sell && sell->isEnabled());
    QTest::mouseClick(sell, Qt::LeftButton);
    GameSaveData sold;
    QVERIFY(SaveManager::load(path(), &sold));
    QCOMPARE(sold.economy.coins, fixture.economy.coins + 30);
    QCOMPARE(sold.economy.inventoryCounts.value(static_cast<int>(CropType::Wheat)), 0);
    QVERIFY(!sell->isEnabled());
}

QTEST_MAIN(GameplayPanelTests)
#include "gameplay_panel_tests.moc"
