#include <QtTest>
#include <algorithm>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QAudioDecoder>
#include <QMediaDevices>
#include <QSoundEffect>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QWheelEvent>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>

#include "app/mainwindow.h"
#include "map/mapdata.h"
#include "storage/savemanager.h"

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
    auto *enter = game.findChild<QPushButton *>("enterFarm");
    QVERIFY(enter && enter->isVisible());
    QTest::mouseClick(enter, Qt::LeftButton);
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
    void wakeSoundResource();
    void wakeSoundDropsUnavailableCues_data();
    void wakeSoundDropsUnavailableCues();
    void wakeSoundPlaysOncePerNotification();
    void wakeSoundWithoutOutput();
    void wakeSoundIgnoresNaturalSeasonChange();
    void homeGuideLeavesFilesUntouched();
    void pauseSaveFailureStaysInFarm();
    void closeSaveFailure_data();
    void closeSaveFailure();
    void unsupportedLegacyFilesRemainAlongsideNewGame();
    void migrateVersionTwo_data();
    void migrateVersionTwo();
    void invalidPlayerPositionRecoversFromBackup();
    void damagedCurrentFilesRemainUntouched();
    void buySelectAndUseFertilizer();
    void sellAtMarket();
    void continueVersionThreeNextToNewFence();
    void fullBackpackAtStorage();
    void newFarmFinancesItsNextCrop();

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

void GameplayPanelTests::wakeSoundResource() {
    QFile wav(QStringLiteral(":/sounds/sleep_success.wav"));
    QVERIFY(wav.open(QIODevice::ReadOnly));
    bool decodedSamples = false;
    bool nonSilent = false;
    QAudioDecoder decoder;
    decoder.setSourceDevice(&wav);
    QSignalSpy finished(&decoder, &QAudioDecoder::finished);
    connect(&decoder, &QAudioDecoder::bufferReady, &decoder, [&] {
        const QAudioBuffer buffer = decoder.read();
        if (!buffer.isValid()) return;
        decodedSamples |= buffer.sampleCount() > 0;
        const auto format = buffer.format();
        const char *samples = buffer.constData<char>();
        for (int i = 0; i < buffer.sampleCount(); ++i) {
            nonSilent |= qAbs(format.normalizedSampleValue(
                samples + i * format.bytesPerSample())) > 0.00001f;
        }
    });
    decoder.start();
    QTRY_VERIFY_WITH_TIMEOUT(finished.count() > 0 || decoder.error() != QAudioDecoder::NoError, 5000);
    QVERIFY2(decoder.error() == QAudioDecoder::NoError, qPrintable(decoder.errorString()));
    QVERIFY(decodedSamples);
    QVERIFY(nonSilent);
}

void GameplayPanelTests::wakeSoundDropsUnavailableCues_data() {
    QTest::addColumn<QString>("mode");
    for (const char *mode : {"muted", "zero-volume", "empty-source", "missing-source"})
        QTest::newRow(mode) << QString::fromLatin1(mode);
}

void GameplayPanelTests::wakeSoundDropsUnavailableCues() {
    QFETCH(QString, mode);
    MainWindow game;
    auto *wake = game.findChild<QSoundEffect *>("wakeSound");
    QVERIFY(wake);
    QCOMPARE(game.findChildren<QSoundEffect *>("wakeSound").size(), 1);
    QCOMPARE(wake->source(), QUrl(QStringLiteral("qrc:/sounds/sleep_success.wav")));
    QCOMPARE(wake->volume(), 0.35f);
    QCOMPARE(wake->loopCount(), 1);
    QTRY_VERIFY(wake->status() != QSoundEffect::Loading);

    const QUrl originalSource = wake->source();
    if (mode == "muted") wake->setMuted(true);
    else if (mode == "zero-volume") wake->setVolume(0);
    else if (mode == "empty-source") wake->setSource(QUrl());
    else wake->setSource(QUrl(QStringLiteral("qrc:/sounds/does-not-exist.wav")));
    QSignalSpy changes(wake, &QSoundEffect::playingChanged);
    // This is an injected notification, not a claim that the baseline save code emits it.
    for (int i = 0; i < 20; ++i) game.wakeSaved();
    QTimer heartbeat;
    heartbeat.setSingleShot(true);
    QSignalSpy heartbeats(&heartbeat, &QTimer::timeout);
    heartbeat.start(0);
    QTRY_COMPARE(heartbeats.count(), 1);
    QTRY_VERIFY(wake->status() != QSoundEffect::Loading);
    QVERIFY(!wake->isPlaying());
    QCOMPARE(changes.count(), 0);

    // Recovering sound later must not replay a stale successful save.
    wake->setMuted(false);
    wake->setVolume(0.0001f);
    wake->setSource(originalSource);
    QTRY_VERIFY(wake->status() != QSoundEffect::Loading);
    QTest::qWait(450);
    QCOMPARE(changes.count(), 0);
    QVERIFY(!QFileInfo::exists(path()));
}

void GameplayPanelTests::wakeSoundPlaysOncePerNotification() {
    if (QMediaDevices::defaultAudioOutput().isNull())
        QSKIP("No output device: resource/failure tests still run; playback requires a device.");
    MainWindow game;
    auto *wake = game.findChild<QSoundEffect *>("wakeSound");
    QVERIFY(wake);
    QTRY_COMPARE(wake->status(), QSoundEffect::Ready);
    wake->setVolume(0.0001f); // Machine playback assertion, not a listening test.
    QSignalSpy changes(wake, &QSoundEffect::playingChanged);
    auto starts = std::make_shared<int>(0);
    connect(wake, &QSoundEffect::playingChanged, wake, [wake, starts] {
        if (wake->isPlaying()) ++*starts;
    });
    // The upstream owner deduplicates each sleep; overlapping notifications are also ignored.
    game.wakeSaved();
    QVERIFY(wake->isPlaying());
    for (int i = 0; i < 20; ++i) game.wakeSaved();
    QCOMPARE(*starts, 1);
    QTRY_VERIFY_WITH_TIMEOUT(!wake->isPlaying(), 2000);
    QTest::qWait(450);
    QCOMPARE(*starts, 1);
    QCOMPARE(changes.count(), 2);
    // A later, distinct successful sleep must still be allowed to sound.
    game.wakeSaved();
    QCOMPARE(*starts, 2);
    QTRY_VERIFY_WITH_TIMEOUT(!wake->isPlaying(), 2000);
    QCOMPARE(changes.count(), 4);
    QVERIFY(!QFileInfo::exists(path()));
}

void GameplayPanelTests::wakeSoundWithoutOutput() {
    if (!QMediaDevices::defaultAudioOutput().isNull())
        QSKIP("Requires a process without a default output device; normal playback is tested separately.");
    GameSaveData before = m_fixture;
    before.gameMinutes = 22 * 60;
    before.playerPosition = stationPosition({25, 8});
    QVERIFY(SaveManager::save(path(), before));
    MainWindow game;
    auto *wake = game.findChild<QSoundEffect *>("wakeSound");
    QVERIFY(wake);
    QTRY_VERIFY(wake->status() != QSoundEffect::Loading);
    wake->setVolume(0.0001f);
    QSignalSpy changes(wake, &QSoundEffect::playingChanged);
    showGame(game);
    auto *map = gameMap(game);
    QVERIFY(map);
    for (int i = 0; i < 20; ++i) game.wakeSaved();
    QVERIFY(QMetaObject::invokeMethod(&game, "sleepUntilMorning", Qt::DirectConnection));
    QTRY_COMPARE_WITH_TIMEOUT(game.currentGameDay(), 1, 4000);
    QTRY_VERIFY_WITH_TIMEOUT(!map->isInputBlocked(), 3000);
    QCOMPARE(game.currentGameHour(), 6);
    QCOMPARE(changes.count(), 0);
    QVERIFY(!wake->isPlaying());
    QVERIFY(game.close());
}

void GameplayPanelTests::wakeSoundIgnoresNaturalSeasonChange() {
    GameSaveData before = m_fixture;
    before.gameMinutes = 27 * 1440 + 1439; // Winter ends naturally at midnight.
    before.lastProcessedDay = 27;
    before.world.lastCompletedDay = 26;
    QVERIFY(SaveManager::save(path(), before));
    MainWindow game;
    auto *wake = game.findChild<QSoundEffect *>("wakeSound");
    QVERIFY(wake);
    wake->setVolume(0.0001f);
    QSignalSpy changes(wake, &QSoundEffect::playingChanged);
    QSignalSpy notifications(&game, &MainWindow::wakeSaved);
    showGame(game);
    auto *map = gameMap(game);
    QVERIFY(map);
    QTRY_COMPARE_WITH_TIMEOUT(game.currentGameDay(), 28, 4000);
    QTRY_VERIFY_WITH_TIMEOUT(!map->isInputBlocked(), 3000);
    QCOMPARE(notifications.count(), 0);
    QCOMPARE(changes.count(), 0);
    QVERIFY(!wake->isPlaying());
    QVERIFY(game.close());
}

void GameplayPanelTests::homeGuideLeavesFilesUntouched() {
    QVERIFY(SaveManager::save(path(), m_fixture));
    const QByteArray before = readBytes(path());
    const QByteArray backup = readBytes(path() + ".bak");
    MainWindow game;
    game.show();
    auto *enter = game.findChild<QPushButton *>("enterFarm");
    auto *guide = game.findChild<QPushButton *>("homeGuide");
    auto *back = game.findChild<QPushButton *>("guideBack");
    QVERIFY(enter && guide && back);
    QCOMPARE(enter->text(), QStringLiteral("继续农场"));
    QTest::mouseClick(guide, Qt::LeftButton);
    QVERIFY(back->isVisible());
    QTest::keyClick(back, Qt::Key_Escape);
    QVERIFY(enter->isVisible());
    QTest::mouseClick(game.findChild<QPushButton *>("exitGame"), Qt::LeftButton);
    QVERIFY(!game.isVisible());
    QCOMPARE(readBytes(path()), before);
    QCOMPARE(readBytes(path() + ".bak"), backup);
}

void GameplayPanelTests::pauseSaveFailureStaysInFarm() {
    QVERIFY(writeBytes(path(), "{torn"));
    MainWindow game;
    auto *notice = game.findChild<QLabel *>("homeNotice");
    QVERIFY(notice && notice->text().contains(QStringLiteral("无法保存")));
    showGame(game);
    auto *view = game.findChild<QGraphicsView *>();
    QTest::keyClick(view, Qt::Key_Escape);
    auto *leave = game.findChild<QPushButton *>("saveReturnHome");
    QVERIFY(leave && leave->isVisible());
    QTest::mouseClick(leave, Qt::LeftButton);
    QVERIFY(leave->isVisible());
    QVERIFY(game.findChild<QLabel *>("pauseNotice")->text().contains(QStringLiteral("未能保存")));
    QCOMPARE(readBytes(path()), QByteArray("{torn"));
}

void GameplayPanelTests::closeSaveFailure_data() {
    QTest::addColumn<bool>("paused");
    QTest::addColumn<bool>("discard");
    QTest::newRow("stay-playing") << false << false;
    QTest::newRow("stay-paused") << true << false;
    QTest::newRow("discard-playing") << false << true;
    QTest::newRow("discard-paused") << true << true;
}

void GameplayPanelTests::closeSaveFailure() {
    QFETCH(bool, paused);
    QFETCH(bool, discard);
    QVERIFY(SaveManager::save(path(), m_fixture));
    const QByteArray original = readBytes(path());
    MainWindow game;
    showGame(game);
    auto *map = gameMap(game);
    auto *view = game.findChild<QGraphicsView *>();
    auto *resume = game.findChild<QPushButton *>("resumeFarm");
    QVERIFY(map && view && resume);

    if (paused) QTest::keyClick(view, Qt::Key_Escape);
    QCOMPARE(resume->isVisible(), paused);
    const QString backup = path() + ".bak";
    QVERIFY(QFile::remove(backup));
    // A directory in place of the backup makes the atomic write fail even as an administrator.
    QVERIFY(QDir().mkdir(backup));
    QVERIFY(!game.close());
    auto *question = game.findChild<QMessageBox *>("closeSaveFailure");
    QVERIFY(question && question->isVisible());
    QVERIFY(map->isInputBlocked());
    QVERIFY(!game.close());
    QCOMPARE(game.findChildren<QMessageBox *>("closeSaveFailure").size(), 1);
    const int minuteAtPrompt = game.currentGameMinute();
    QTest::qWait(1100);
    QCOMPARE(game.currentGameMinute(), minuteAtPrompt);

    QVERIFY(QDir().rmdir(backup));
    auto *choice = question->findChild<QPushButton *>(discard ? "discardAndQuit" : "keepFarmOpen");
    QVERIFY(choice);
    QTest::mouseClick(choice, Qt::LeftButton);
    QTRY_VERIFY(!QApplication::activeModalWidget());
    QCOMPARE(game.isVisible(), !discard);
    QVERIFY(!QFileInfo::exists(backup)); // Neither decision performs an extra save.
    QCOMPARE(readBytes(path()), original);
    if (discard) return;
    QCOMPARE(resume->isVisible(), paused);
    QCOMPARE(map->isInputBlocked(), paused);
    if (paused) {
        QTest::qWait(1100);
        QCOMPARE(game.currentGameMinute(), minuteAtPrompt);
        QTest::keyClick(resume, Qt::Key_Escape);
        QVERIFY(!map->isInputBlocked());
    }
    QTRY_VERIFY_WITH_TIMEOUT(game.currentGameMinute() != minuteAtPrompt, 2000);
    QVERIFY(game.close());
    GameSaveData last;
    QVERIFY(SaveManager::load(path(), &last));
    QCOMPARE(last.gameMinutes % 60, game.currentGameMinute());
    QVERIFY(SaveManager::load(backup, &last));
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
    const int wheat = static_cast<int>(CropType::Wheat);
    const int beet = static_cast<int>(CropType::Beet);
    fixture.economy.inventoryCounts[wheat] = 6;
    fixture.economy.inventoryCounts[beet] = 1;
    fixture.economy.storage.harvestCounts[beet] = 3;
    QVERIFY(SaveManager::save(path(), fixture));
    MainWindow game;
    showGame(game);
    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_E);
    QTRY_VERIFY(panel("marketDialog"));
    auto *sell = panel("marketDialog")->findChild<QPushButton *>("sellAll");
    auto *sellWheat = panel("marketDialog")->findChild<QPushButton *>("sell_wheat");
    auto *sellBeet = panel("marketDialog")->findChild<QPushButton *>("sell_beet");
    auto *quantity = panel("marketDialog")->findChild<QSpinBox *>("sellQuantity_wheat");
    QVERIFY(sell && sellWheat && sellBeet && quantity);
    GameSaveData sold;
    QCOMPARE(quantity->value(), 1);
    QCOMPARE(quantity->maximum(), 6);
    quantity->setFocus();
    quantity->selectAll();
    QTest::keyClicks(quantity, "4");
    QTest::keyClick(quantity, Qt::Key_Return);
    QCOMPARE(sellWheat->text(), QStringLiteral("卖出 · 20 金币"));
    QVERIFY(SaveManager::load(path(), &sold));
    QCOMPARE(sold.economy.coins, fixture.economy.coins);
    QTest::mouseClick(sellWheat, Qt::LeftButton);
    QVERIFY(SaveManager::load(path(), &sold));
    QCOMPARE(sold.economy.inventoryCounts.value(wheat), 2);
    QCOMPARE(sold.economy.inventoryCounts.value(beet), 1);
    QCOMPARE(sold.economy.coins, fixture.economy.coins + 20);
    QCOMPARE(quantity->maximum(), 2);
    QCOMPARE(quantity->value(), 2);
    QTest::mouseClick(sellBeet, Qt::LeftButton);
    QVERIFY(!sellBeet->isEnabled());
    QTest::mouseClick(sellBeet, Qt::LeftButton);
    QVERIFY(SaveManager::load(path(), &sold));
    QCOMPARE(sold.economy.inventoryCounts.value(wheat), 2);
    QCOMPARE(sold.economy.inventoryCounts.value(beet), 0);
    QCOMPARE(sold.economy.coins, fixture.economy.coins + 24);
    QVERIFY(sell->isEnabled());
    QTest::mouseClick(sell, Qt::LeftButton);
    QVERIFY(SaveManager::load(path(), &sold));
    QCOMPARE(sold.economy.coins, fixture.economy.coins + 34);
    QCOMPARE(sold.economy.inventoryCounts.value(wheat), 0);
    QCOMPARE(sold.economy.storage.harvestCounts.value(beet), 3);
    for (CropType crop : CropCatalog::cropTypes()) {
        const int id = static_cast<int>(crop);
        QCOMPARE(sold.economy.seedCounts.value(id), fixture.economy.seedCounts.value(id));
    }
    QVERIFY(!sellWheat->isEnabled());
    QVERIFY(!sell->isEnabled());
}

void GameplayPanelTests::continueVersionThreeNextToNewFence() {
    GameSaveData before = m_fixture;
    before.playerPosition = stationPosition({28, 14});
    const QString sample = m_directory->filePath("old-fixture.json");
    QVERIFY(SaveManager::save(sample, before));
    QJsonObject legacy = QJsonDocument::fromJson(readBytes(sample)).object();
    legacy["schemaVersion"] = 3;
    QJsonObject assets = legacy["economy"].toObject();
    assets.remove("storage");
    legacy["economy"] = assets;
    const QByteArray bytes = QJsonDocument(legacy).toJson();
    QVERIFY(writeBytes(SaveManager::previousSavePath(), bytes));
    MainWindow game;
    showGame(game);
    auto *map = gameMap(game);
    QVERIFY(map);
    auto *player = map->findChild<ImageMovementWidget *>();
    QVERIFY(player);
    const QPoint tile = player->currentTilePosition();
    QVERIFY(tile != QPoint(28, 14));
    QCOMPARE((tile - QPoint(28, 14)).manhattanLength(), 1);
    QCOMPARE(map->collisionLayer().at(tile.y()).at(tile.x()), 0);
    QVERIFY(game.close());
    GameSaveData after;
    QVERIFY(SaveManager::load(path(), &after));
    QCOMPARE(after.playerPosition, player->absolutePosition());
    QCOMPARE(after.economy.coins, before.economy.coins);
    QCOMPARE(after.economy.storage.fertilizerCount, 0);
    QCOMPARE(readBytes(SaveManager::previousSavePath()), bytes);
}

void GameplayPanelTests::fullBackpackAtStorage() {
    const int wheat = static_cast<int>(CropType::Wheat);
    GameSaveData before = m_fixture;
    before.playerPosition = stationPosition({31, 21});
    before.economy.inventoryCounts[wheat] = 11;
    before.economy.storage.harvestCounts[wheat] = 2;
    QVERIFY(SaveManager::save(path(), before));
    MainWindow game;
    showGame(game);
    auto *map = gameMap(game);
    QVERIFY(map);
    auto *player = map->findChild<ImageMovementWidget *>();
    QVERIFY(player);
    QTest::keyClick(player, Qt::Key_E);
    QTRY_VERIFY(panel("storageDialog"));
    auto *dialog = panel("storageDialog");
    auto *all = dialog->findChild<QPushButton *>("withdrawAll");
    auto *one = dialog->findChild<QPushButton *>("withdrawOne");
    QVERIFY(all && one && all->isEnabled() && one->isEnabled());
    QTest::mouseClick(all, Qt::LeftButton);
    const auto labels = dialog->findChildren<QLabel *>();
    QVERIFY(std::any_of(labels.cbegin(), labels.cend(),
                        [](QLabel *label) { return label->text().contains(QStringLiteral("只剩 1 格")); }));
    GameSaveData after;
    QVERIFY(SaveManager::load(path(), &after));
    QCOMPARE(after.economy.inventoryCounts.value(wheat), 11);
    QCOMPARE(after.economy.storage.harvestCounts.value(wheat), 2);
    QTest::mouseClick(one, Qt::LeftButton);
    QVERIFY(SaveManager::load(path(), &after));
    QCOMPARE(after.economy.inventoryCounts.value(wheat), 12);
    QCOMPARE(after.economy.storage.harvestCounts.value(wheat), 1);
    dialog->close();
    QVERIFY(game.close());

    // The market sells only what the player carries, even after a reload.
    after.playerPosition = stationPosition({28,18});
    QVERIFY(SaveManager::save(path(), after));
    MainWindow reopened;
    showGame(reopened);
    auto *marketMap = gameMap(reopened);
    QVERIFY(marketMap);
    auto *seller = marketMap->findChild<ImageMovementWidget *>();
    QVERIFY(seller);
    QTest::keyClick(seller, Qt::Key_E);
    QTRY_VERIFY(panel("marketDialog"));
    auto *sell = panel("marketDialog")->findChild<QPushButton *>("sellAll");
    QVERIFY(sell && sell->isEnabled());
    QTest::mouseClick(sell, Qt::LeftButton);
    QVERIFY(reopened.close());
    QVERIFY(SaveManager::load(path(), &after));
    QCOMPARE(after.economy.inventoryCounts.value(wheat), 0);
    QCOMPARE(after.economy.storage.harvestCounts.value(wheat), 1);
    QCOMPARE(after.economy.coins, before.economy.coins + 60);
}

void GameplayPanelTests::newFarmFinancesItsNextCrop() {
    const QVector<QPoint> field{{22,17}, {23,17}, {24,17}, {22,18}, {23,18}, {24,18}};
    std::unique_ptr<MainWindow> game;
    MapWidget *map = nullptr;
    ItemBar *bar = nullptr;
    ImageMovementWidget *player = nullptr;

    const auto openFarm = [&] {
        game = std::make_unique<MainWindow>();
        showGame(*game);
        map = gameMap(*game);
        if (!map) return false;
        bar = map->window()->findChild<ItemBar *>();
        player = map->findChild<ImageMovementWidget *>();
        return bar && player;
    };
    // Fast-forward only the time of day in the temporary save. Each midnight
    // and every crop/asset change still goes through the real game window.
    const auto reopenAt = [&](int minuteOfDay) {
        if (!game->close()) return false;
        game.reset();
        GameSaveData state;
        if (!SaveManager::load(path(), &state)) return false;
        state.gameMinutes = (state.gameMinutes / 1440) * 1440 + minuteOfDay;
        return SaveManager::save(path(), state) && openFarm();
    };
    const auto wheelTo = [&](ToolType tool) {
        for (int i = 0; i < ToolbarSlotCount && bar->selectedTool() != tool; ++i) {
            const QPoint pos = bar->rect().center();
            QWheelEvent wheel(pos, bar->mapToGlobal(pos), {}, {0,-120},
                              Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(bar, &wheel);
        }
        return bar->selectedTool() == tool && player->getCurrentTool().type == tool;
    };
    const auto clickField = [&](QPoint tile) {
        auto *view = game->findChild<QGraphicsView *>();
        QWidget *canvas = map->window();
        if (!view || !canvas->graphicsProxyWidget()) return false;
        const QPoint point(tile.x() * map->tileWidth() + map->tileWidth() / 2 - map->offsetX(),
                           tile.y() * map->tileHeight() + map->tileHeight() / 2 - map->offsetY());
        const QPoint screen = view->mapFromScene(canvas->graphicsProxyWidget()->mapToScene(map->mapTo(canvas, point)));
        if (!view->viewport()->rect().contains(screen)) return false;
        QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, screen);
        return true;
    };
    const auto walkTo = [&](Qt::Key key, int tile) {
        const bool horizontal = key == Qt::Key_A || key == Qt::Key_D;
        QTest::keyPress(player, key);
        const bool arrived = QTest::qWaitFor([&] {
            const QPoint pos = player->currentTilePosition();
            return (horizontal ? pos.x() : pos.y()) == tile;
        }, 4000);
        QTest::keyRelease(player, key);
        return arrived;
    };
    const auto pressGameKey = [](Qt::Key key) {
        // Modal activation is asynchronous on both Cocoa and the offscreen plugin.
        // Deliver the key after Qt has moved keyboard focus into the visible panel.
        QDialog *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        const bool ready = QTest::qWaitFor([&] {
            QWidget *focus = QApplication::focusWidget();
            return focus && (!modal || focus->window() == modal);
        }, 2000);
        if (!ready) {
            qWarning() << "Keyboard focus" << QApplication::focusWidget() << "modal" << modal;
            return false;
        }
        QTest::keyClick(QApplication::focusWidget(), key);
        return true;
    };
    const auto selectFromBag = [&](int category, const QString &listName, ToolType tool) {
        if (!pressGameKey(Qt::Key_Tab) || !QTest::qWaitFor([&]{ return panel("inventoryDialog") != nullptr; }, 1500))
            return false;
        QDialog *bag = panel("inventoryDialog");
        auto *categories = bag->findChild<QTabWidget *>("bagCategories");
        auto *list = bag->findChild<QListWidget *>(listName);
        if (!categories || !list) return false;
        auto *tabs = categories->tabBar();
        QTest::mouseClick(tabs, Qt::LeftButton, {}, tabs->tabRect(category).center());
        for (int i = 0; i < list->count(); ++i) {
            const auto *item = list->item(i);
            if (item->isHidden() || item->data(Qt::UserRole).toInt() != static_cast<int>(tool)) continue;
            QTest::mouseClick(list->viewport(), Qt::LeftButton, {}, list->visualItemRect(item).center());
            return QTest::qWaitFor([&]{ return !panel("inventoryDialog") && bar->selectedTool() == tool; }, 1500);
        }
        return false;
    };

    QVERIFY(openFarm());
    QVERIFY(wheelTo(ToolType::Hoe));
    for (const auto &tile : field) {
        QVERIFY(clickField(tile));
        QCOMPARE(map->worldSnapshot().cultivatedLayer.at(tile.y()).at(tile.x()), GameConstants::CultivatedTileGid);
    }
    QVERIFY(wheelTo(ToolType::Wheat));
    for (const auto &tile : field) QVERIFY(clickField(tile));
    GameSaveData state;
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.seedCounts.value(static_cast<int>(CropType::Wheat)), 0);
    QCOMPARE(state.world.crops.size(), 6);

    for (int day = 0; day < 3; ++day) {
        QVERIFY(wheelTo(ToolType::WateringCan));
        for (const auto &tile : field) QVERIFY(clickField(tile));
        QCOMPARE(map->worldSnapshot().wateredTiles.size(), 6);
        QVERIFY(reopenAt(1439));
        QTRY_COMPARE_WITH_TIMEOUT(map->worldSnapshot().lastCompletedDay, day, 4000);
        for (const auto &crop : map->worldSnapshot().crops) QCOMPARE(crop.growthDays, day + 1);
    }

    QVERIFY(wheelTo(ToolType::EmptyHand));
    for (const auto &tile : field) QVERIFY(clickField(tile));
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.inventoryCounts.value(static_cast<int>(CropType::Wheat)), 6);
    QCOMPARE(state.world.crops.size(), 0);
    QVERIFY(walkTo(Qt::Key_S, 18));
    // Visit the real chest from the farm, then return through the home screen.
    QVERIFY(walkTo(Qt::Key_D, 30));
    QVERIFY(walkTo(Qt::Key_S, 21));
    QVERIFY(walkTo(Qt::Key_D, 31));
    QVERIFY(pressGameKey(Qt::Key_E));
    QTRY_VERIFY(panel("storageDialog"));
    auto *deposit = panel("storageDialog")->findChild<QPushButton *>("depositAll");
    QVERIFY(deposit && deposit->isEnabled());
    QTest::mouseClick(deposit, Qt::LeftButton);
    const int wheat = static_cast<int>(CropType::Wheat);
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.storage.harvestCounts.value(wheat), 6);
    QCOMPARE(state.economy.inventoryCounts.value(wheat), 0);
    QVERIFY(pressGameKey(Qt::Key_Escape));
    QTRY_VERIFY(!panel("storageDialog") && !map->isInputBlocked());
    QVERIFY(pressGameKey(Qt::Key_Escape));
    auto *saveHome = game->findChild<QPushButton *>("saveReturnHome");
    QTRY_VERIFY(saveHome && saveHome->isVisible());
    QTest::mouseClick(saveHome, Qt::LeftButton);
    auto *enter = game->findChild<QPushButton *>("enterFarm");
    QTRY_VERIFY(enter && enter->isVisible());
    QTest::mouseClick(enter, Qt::LeftButton);
    QTRY_VERIFY(!map->isInputBlocked());
    QCOMPARE(player->currentTilePosition(), QPoint(31,21));
    QVERIFY(pressGameKey(Qt::Key_E));
    QTRY_VERIFY(panel("storageDialog"));
    auto *withdraw = panel("storageDialog")->findChild<QPushButton *>("withdrawAll");
    QVERIFY(withdraw && withdraw->isEnabled());
    QTest::mouseClick(withdraw, Qt::LeftButton);
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.storage.harvestCounts.value(wheat), 0);
    QCOMPARE(state.economy.inventoryCounts.value(wheat), 6);
    QVERIFY(pressGameKey(Qt::Key_Escape));
    QTRY_VERIFY(!panel("storageDialog"));
    QVERIFY(walkTo(Qt::Key_A, 29)); // Leave enough space for the player's feet to clear the chest.
    QVERIFY(walkTo(Qt::Key_W, 18));
    QVERIFY(walkTo(Qt::Key_A, 28));
    QVERIFY(pressGameKey(Qt::Key_E));
    QTRY_VERIFY(panel("marketDialog"));
    auto *sell = panel("marketDialog")->findChild<QPushButton *>("sellAll");
    QVERIFY(sell && sell->isEnabled());
    QTest::mouseClick(sell, Qt::LeftButton);
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.coins, 30);
    QVERIFY(pressGameKey(Qt::Key_Escape));
    QTRY_VERIFY(!panel("marketDialog"));

    QVERIFY(walkTo(Qt::Key_D, 33));
    QVERIFY(pressGameKey(Qt::Key_E));
    QTRY_VERIFY(panel("shopDialog"));
    for (const char *id : {"buy_carrot_1", "buy_fertilizer_1"}) {
        auto *buy = panel("shopDialog")->findChild<QPushButton *>(id);
        QVERIFY(buy && buy->isEnabled());
        QTest::mouseClick(buy, Qt::LeftButton);
    }
    QVERIFY(pressGameKey(Qt::Key_Escape));
    QTRY_VERIFY(!panel("shopDialog"));
    QVERIFY(selectFromBag(1, QStringLiteral("bagSeeds"), ToolType::Carrot));
    QVERIFY(clickField(field.first()));
    QVERIFY(selectFromBag(2, QStringLiteral("bagFertilizer"), ToolType::Fertilizer));
    QVERIFY(clickField(field.at(1))); // Empty cultivated ground must cost nothing.
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.fertilizerCount, 1);
    QVERIFY(clickField(field.first()));
    QVERIFY(wheelTo(ToolType::WateringCan));
    QVERIFY(clickField(field.first()));
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.coins, 27);
    QCOMPARE(state.economy.fertilizerCount, 0);
    QCOMPARE(state.world.crops.size(), 1);
    QVERIFY(state.world.crops.first().fertilizerBonusPending);

    QVERIFY(reopenAt(22 * 60));
    QVERIFY(walkTo(Qt::Key_A, 26));
    QVERIFY(walkTo(Qt::Key_W, 8));
    QVERIFY(pressGameKey(Qt::Key_E));
    QTRY_COMPARE_WITH_TIMEOUT(game->currentGameDay(), 4, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!map->isInputBlocked(), 3000);
    QCOMPARE(game->currentGameHour(), 6);
    QVERIFY(game->close());
    game.reset();
    QVERIFY(openFarm());
    QCOMPARE(game->currentGameDay(), 4);
    QCOMPARE(map->worldSnapshot().crops.size(), 1);
    const auto crop = map->worldSnapshot().crops.first();
    QCOMPARE(crop.type, CropType::Carrot);
    QCOMPARE(crop.growthDays, 2);
    QVERIFY(crop.fertilizerApplied);
    QVERIFY(!crop.fertilizerBonusPending);
    QVERIFY(game->close());
    QVERIFY(SaveManager::load(path(), &state));
    QCOMPARE(state.economy.coins, 27);
    QCOMPARE(state.economy.fertilizerCount, 0);
}

QTEST_MAIN(GameplayPanelTests)
#include "gameplay_panel_tests.moc"
