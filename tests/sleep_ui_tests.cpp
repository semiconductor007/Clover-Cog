#include <QtTest>
#include <QTemporaryDir>
#include <QPushButton>
#include <QShortcut>
#include "daynightenvironment.h"
#include <QFontMetrics>
#include <QRawFont>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QtEndian>
#include <limits>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "mainwindow.h"

class SleepFlowTests : public QObject {
    Q_OBJECT
public:
    static int previewSleep() {
        QTemporaryDir dir;
        const bool dayNight = QApplication::arguments().contains(QStringLiteral("--preview-daynight"));
        const bool calendar = QApplication::arguments().contains(QStringLiteral("--preview-calendar"));
        const bool confirmation = QApplication::arguments().contains(QStringLiteral("--preview-confirm"));
        const bool passout = QApplication::arguments().contains(QStringLiteral("--preview-passout"));
        const bool early = QApplication::arguments().contains(QStringLiteral("--early"));
        const bool failSave = QApplication::arguments().contains(QStringLiteral("--save-failure"));
        MainWindow window(nullptr, failSave ? dir.path() : dir.filePath("preview.json"));
        SleepFlowTests fixture;
        fixture.prepare(window, early ? 1440 + 120 : 22 * 60);
        if (passout) {
            window.m_gameClock.setElapsedGameMinutes(1559);
            window.m_lastObservedGameMinutes = 1559;
            window.character->setInitialPosition(QPoint(100, 100));
            window.updateTime();
        }
        if (confirmation) {
            window.m_gameClock.setElapsedGameMinutes(720);
            window.character->setInitialPosition(QPoint(256, 176));
            window.updateTime();
        }
        if (QApplication::arguments().contains(QStringLiteral("--with-sound"))) window.setEffectsMuted(false);
        if (calendar) {
            window.m_gameClock.setElapsedGameMinutes(6 * 1440LL + 1320);
            auto world = window.mapWidget->worldSnapshot();
            world.lastCompletedDay = 5;
            world.crops[0].plantedDay = 6;
            window.mapWidget->restoreWorld(world);
            window.m_lastProcessedDay = 6;
            window.m_lastObservedGameMinutes = window.m_gameClock.elapsedGameMinutes();
            window.updateTime();
        }
        window.m_saveEnabled = true;
        if (early) window.updateTime();
        window.setWindowTitle(QStringLiteral("Clover-Cog Sleep Preview - %1")
            .arg(passout ? QStringLiteral("Pass-out at 02:00")
                         : confirmation ? QStringLiteral("Daytime confirmation")
                         : calendar ? QStringLiteral("Calendar spring to summer")
                         : dayNight ? QStringLiteral("Day-night F6/F7/F8/F9")
                         : failSave ? QStringLiteral("Save failure")
                         : early ? QStringLiteral("Early morning") : QStringLiteral("Normal")));
        window.resize(1040, 640);
        window.setWindowFlag(Qt::WindowStaysOnTopHint);
        window.show();
        if (dayNight) {
            const auto addTimeKey = [&window](Qt::Key key, int minute) {
                auto *shortcut = new QShortcut(QKeySequence(key), &window);
                QObject::connect(shortcut, &QShortcut::activated, &window, [&window, minute] {
                    if (window.m_sleepBusy) return;
                    window.m_gameClock.setElapsedGameMinutes(window.currentGameDay() * 1440LL + minute);
                    window.updateTime();
                });
            };
            addTimeKey(Qt::Key_F6, 720);
            addTimeKey(Qt::Key_F7, 1140);
            addTimeKey(Qt::Key_F8, 1320);
            addTimeKey(Qt::Key_F9, 360);
            window.updateTime();
        } else {
            QTimer::singleShot(400, &window, [&window, confirmation, passout] {
                if (passout) { window.m_gameClock.setElapsedGameMinutes(1560); window.updateTime(); }
                else if (confirmation) window.requestSleep();
                else window.sleepUntilMorning();
            });
        }
        return QApplication::exec();
    }
private:
    void prepare(MainWindow &window, qint64 minutes) {
        window.m_audioOutput->setVolume(0);
        window.m_sleepSound->setMuted(true);
        window.m_gameClock.pause();
        window.m_gameClock.setElapsedGameMinutes(minutes);
        window.m_lastObservedGameMinutes = minutes; // Fixture snapshot, not elapsed play.
        WorldStateSnapshot world = window.mapWidget->worldSnapshot();
        world.crops = {{QPoint(5, 5), CropType::Wheat, 0, 0}};
        world.cultivatedLayer[5][5] = GameConstants::CultivatedTileGid;
        world.wateredTiles = {QPoint(5, 5)};
        world.lastCompletedDay = -1;
        QVERIFY(window.mapWidget->restoreWorld(world));
        window.m_lastProcessedDay = 0;
    }
private slots:
    void blockedBedKeepsCheckpointAndCanRetrySafely() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("blocked.json"));
        prepare(window, 119);
        window.character->setInitialPosition(QPoint(100, 100));
        QVERIFY(window.saveGame());
        QFile checkpointFile(window.m_savePath);
        QVERIFY(checkpointFile.open(QIODevice::ReadOnly));
        const QByteArray checkpoint = checkpointFile.readAll(); checkpointFile.close();
        const auto farm = window.mapWidget->worldSnapshot();
        const int width = window.mapWidget->mapWidth(), height = window.mapWidget->mapHeight();
        const auto writeMap = [&](const QString &path, int collision) {
            QJsonArray tiles;
            for (int i = 0; i < width * height; ++i) tiles.append(collision);
            QJsonObject layer{{"name", "collision"}, {"type", "tilelayer"},
                              {"width", width}, {"height", height}, {"data", tiles}};
            QJsonObject map{{"width", width}, {"height", height}, {"tilewidth", 16}, {"tileheight", 16},
                            {"layers", QJsonArray{layer}}, {"tilesets", QJsonArray{}}};
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return false;
            return file.write(QJsonDocument(map).toJson()) > 0;
        };
        const QString blocked = dir.filePath("blocked.tmj"), clear = dir.filePath("clear.tmj");
        QVERIFY(writeMap(blocked, 1));
        QVERIFY(window.mapWidget->loadMap(blocked));
        QVERIFY(window.mapWidget->restoreWorld(farm));
        window.m_gameClock.setElapsedGameMinutes(120);
        window.updateTime();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Failed, 3000);
        QCOMPARE(window.character->absolutePosition(), QPoint(100, 100));
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 120LL);
        QVERIFY(!window.saveGame());
        QVERIFY(checkpointFile.open(QIODevice::ReadOnly));
        QCOMPARE(checkpointFile.readAll(), checkpoint); checkpointFile.close();
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Return);
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Failed);
        QVERIFY(writeMap(clear, 0));
        QVERIFY(window.mapWidget->loadMap(clear));
        QVERIFY(window.mapWidget->restoreWorld(farm));
        window.m_sleepOverlay->retrySaveRequested();
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 360LL);
        QVERIFY(window.m_bedArea.contains(window.character->currentTilePosition()));
        QVERIFY(window.character->canStandAt(window.character->absolutePosition()));
        QVERIFY(window.m_sleepSaved);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 0);
    }

    void passOutAtTwoReturnsToBedAndDoesNotRepeatMidnight() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("passout.json"));
        prepare(window, 1439);
        window.character->setInitialPosition(QPoint(100, 100));
        auto assets = window.m_economy.state();
        assets.coins = 120;
        assets.inventoryCounts.insert(static_cast<int>(CropType::Wheat), 2);
        QVERIFY(window.m_economy.restore(assets));
        window.updateHud();
        const auto money = window.m_economy.state();
        window.resize(1040, 640);
        window.show();
        QTest::qWait(50);
        window.m_gameClock.setElapsedGameMinutes(1559);
        window.updateTime();
        QVERIFY(!window.m_sleepBusy);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        window.toggleInventory();
        QVERIFY(window.m_inventoryPanel->isVisible());
        window.m_gameClock.setElapsedGameMinutes(1560);
        window.updateTime();
        QVERIFY(window.m_sleepBusy);
        QVERIFY(window.m_passedOut);
        QVERIFY(!window.m_sleepConfirming);
        QVERIFY(window.m_inventoryPanel->isHidden());
        QVERIFY(!window.mapWidget->isEnabled());
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1560LL);
        window.updateTime();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(window.m_lastProcessedDay, 1);
        QCOMPARE(window.mapWidget->worldSnapshot().lastCompletedDay, 0);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QVERIFY(window.m_bedArea.contains(window.character->currentTilePosition()));
        QVERIFY(window.character->canStandAt(window.character->absolutePosition()));
        QVERIFY(!window.character->canStandAt(QPoint(-1, -1)));
        QVERIFY(!window.character->canStandAt(QPoint(std::numeric_limits<int>::max(), 0)));
        QCOMPARE(window.m_economy.state().coins, money.coins);
        QCOMPARE(window.m_economy.state().inventoryCounts, money.inventoryCounts);
        QCOMPARE(window.m_economy.state().seedCounts, money.seedCounts);
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/passout-morning.png")));
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.gameMinutes, 1800LL);
        QCOMPARE(saved.playerPosition, window.character->absolutePosition());
        MainWindow loaded(nullptr, window.m_savePath);
        loaded.m_audioOutput->setVolume(0);
        loaded.m_gameClock.pause();
        QCOMPARE(loaded.character->absolutePosition(), saved.playerPosition);
        QVERIFY(!loaded.m_sleepBusy);
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!window.m_sleepBusy, 3000);
        window.m_gameClock.pause();
        window.updateTime();
        QVERIFY(!window.m_sleepBusy);
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/passout-awake.png")));
    }

    void passOutSaveFailureRetriesSameBedCheckpoint() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.path());
        prepare(window, 1559);
        window.m_saveEnabled = true;
        window.character->setInitialPosition(QPoint(100, 100));
        window.updateTime();
        window.m_gameClock.setElapsedGameMinutes(1560);
        window.updateTime();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QVERIFY(!window.m_sleepSaved);
        const QPoint position = window.character->absolutePosition();
        QVERIFY(window.m_bedArea.contains(window.character->currentTilePosition()));
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        window.m_savePath = dir.filePath("recovered.json");
        window.m_sleepOverlay->retrySaveRequested();
        QVERIFY(window.m_sleepSaved);
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.playerPosition, position);
        QCOMPARE(saved.gameMinutes, 1800LL);
        QCOMPARE(saved.world.crops.first().growthDays, 1);
    }

    void delayedTickStopsAtFirstPassOutAndShopCloses() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("delay.json"));
        prepare(window, 1439);
        window.character->setInitialPosition(QPoint(100, 100));
        QTimer::singleShot(20, &window, [&window] {
            window.m_gameClock.setElapsedGameMinutes(3 * 1440LL + 540);
            window.updateTime();
        });
        window.openSeedShop(); // Nested modal event loop must not postpone pass-out.
        QVERIFY(window.m_activeShop.isNull());
        QVERIFY(window.m_passedOut);
        QCOMPARE(window.m_sleepStartMinutes, 1560LL);
        QCOMPARE(window.m_sleepTarget, 1800LL);
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(window.m_lastProcessedDay, 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QVERIFY(window.m_bedArea.contains(window.character->currentTilePosition()));
    }

    void overdueSaveForcesRecoveryAfterAllWidgetsExist() {
        QTemporaryDir dir;
        const QString path = dir.filePath("overdue.json");
        MainWindow source(nullptr, dir.filePath("fixture.json"));
        prepare(source, 1620);
        GameSaveData data;
        data.gameMinutes = 1620;
        data.lastProcessedDay = 0;
        data.world = source.mapWidget->worldSnapshot();
        data.economy = source.m_economy.state();
        data.playerPosition = QPoint(100, 100);
        QVERIFY(SaveManager::save(path, data));
        MainWindow loaded(nullptr, path);
        loaded.m_audioOutput->setVolume(0);
        loaded.m_sleepSound->setMuted(true);
        QVERIFY(loaded.m_passedOut);
        QVERIFY(loaded.m_sleepBusy);
        QVERIFY(loaded.m_gameClock.isPaused());
        QTRY_COMPARE_WITH_TIMEOUT(loaded.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(loaded.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(loaded.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QVERIFY(loaded.m_bedArea.contains(loaded.character->currentTilePosition()));
    }

    void voluntaryRestAt0159AvoidsPassOut() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("manual.json"));
        prepare(window, 1559);
        window.character->setInitialPosition(QPoint(256, 176));
        window.requestSleep();
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Confirming);
        window.confirmSleep();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QVERIFY(!window.m_passedOut);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void bedConfirmationCancelRestoresState_data() {
        QTest::addColumn<bool>("running");
        QTest::newRow("already-paused") << false;
        QTest::newRow("running") << true;
    }

    void bedConfirmationCancelRestoresState() {
        QFETCH(bool, running);
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("cancel.json"));
        prepare(window, 720);
        window.character->setInitialPosition(QPoint(256, 176));
        QVERIFY(window.m_bedArea.contains(window.character->currentTilePosition()));
        QVERIFY(window.saveGame());
        QFile file(window.m_savePath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray checkpoint = file.readAll(); file.close();
        window.resize(1040, 640);
        window.show();
        QTest::qWait(50);
        if (running) window.m_gameClock.resume();
        window.updateTime();
        QVERIFY(window.m_bedHintLabel->isVisible());
        QSignalSpy accepted(window.m_sleepSound, &SleepSoundCue::successAccepted);
        QTest::keyClick(window.character, Qt::Key_E);
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Confirming);
        QVERIFY(window.m_sleepBusy);
        QVERIFY(!window.mapWidget->isEnabled());
        const qint64 selected = window.m_gameClock.elapsedGameMinutes();
        QTest::qWait(1100);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), selected);
        QVERIFY(!window.saveGame());
        window.toggleInventory();
        QVERIFY(window.m_inventoryPanel->isHidden());
        QKeyEvent repeat(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString(), true);
        QApplication::sendEvent(window.m_sleepOverlay, &repeat);
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Confirming);
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/sleep-confirmation.png")));
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Escape);
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Hidden);
        QVERIFY(!window.m_sleepBusy);
        QCOMPARE(window.m_gameClock.isPaused(), !running);
        QVERIFY(window.mapWidget->isEnabled());
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 0);
        QCOMPARE(accepted.count(), 0);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), checkpoint);
        const QPoint position = window.character->absolutePosition();
        QTest::keyPress(window.character, Qt::Key_D);
        QTest::qWait(180);
        QTest::keyRelease(window.character, Qt::Key_D);
        QVERIFY(window.character->absolutePosition() != position);
    }

    void confirmationRevalidatesBedAndSuccessCueIsOnce() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("confirm.json"));
        prepare(window, 720);
        window.character->setInitialPosition(QPoint(256, 176));
        QSignalSpy accepted(window.m_sleepSound, &SleepSoundCue::successAccepted);
        window.requestSleep();
        QPushButton *cancel = nullptr;
        for (auto *button : window.m_sleepOverlay->findChildren<QPushButton *>())
            if (button->text().contains(QStringLiteral("CANCEL"))) cancel = button;
        QVERIFY(cancel);
        QTest::keyClick(cancel, Qt::Key_Return); // Enter on Cancel must not start sleep.
        QVERIFY(!window.m_sleepBusy);
        window.requestSleep();
        window.character->setInitialPosition(QPoint(100, 100)); // Scene/input owner moved the player.
        window.m_sleepOverlay->confirmed();
        QVERIFY(!window.m_sleepBusy);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 720LL);
        QVERIFY(!QFileInfo::exists(window.m_savePath));
        QCOMPARE(accepted.count(), 0);
        window.character->setInitialPosition(QPoint(256, 176));
        window.requestSleep();
        QPushButton *sleepButton = nullptr;
        for (auto *button : window.m_sleepOverlay->findChildren<QPushButton *>())
            if (button->text().contains(QStringLiteral("SLEEP [ENTER]"))) sleepButton = button;
        QVERIFY(sleepButton);
        QTest::keyClick(sleepButton, Qt::Key_Return);
        window.m_sleepOverlay->confirmed(); // A queued duplicate confirmation is ignored.
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QCOMPARE(accepted.count(), 1);
        window.m_sleepOverlay->retrySaveRequested();
        window.updateTime();
        QCOMPARE(accepted.count(), 1);
        QPushButton *wake = nullptr;
        for (auto *button : window.m_sleepOverlay->findChildren<QPushButton *>())
            if (button->text().contains(QStringLiteral("WAKE UP"))) wake = button;
        QVERIFY(wake);
        QKeyEvent repeatedReturn(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString(), true);
        QApplication::sendEvent(wake, &repeatedReturn);
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning);
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!window.m_sleepBusy, 3000);
        window.m_gameClock.pause();
        window.requestSleep(); // A deliberate new sleep at 06:00 is allowed.
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Confirming);
        window.confirmSleep();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.currentGameDay(), 2);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1); // No second watering.
        QCOMPARE(accepted.count(), 2);
    }

    void sleepCueHandlesMuteMissingSourceAndRepeatedResults() {
        QFile wav(QStringLiteral(":/sounds/wav/sleep_success.wav"));
        QVERIFY(wav.open(QIODevice::ReadOnly));
        const QByteArray bytes = wav.readAll();
        QCOMPARE(bytes.left(4), QByteArray("RIFF"));
        QCOMPARE(bytes.mid(8, 4), QByteArray("WAVE"));
        QVERIFY(bytes.size() > 44);
        const auto *data = reinterpret_cast<const uchar *>(bytes.constData());
        QCOMPARE(qFromLittleEndian<quint16>(data + 22), quint16(1));
        QCOMPARE(qFromLittleEndian<quint32>(data + 24), quint32(22050));
        QCOMPARE(qFromLittleEndian<quint16>(data + 34), quint16(16));
        QCOMPARE(qFromLittleEndian<quint32>(data + 40) + 44, quint32(bytes.size()));
        SleepSoundCue cue;
        cue.setMuted(true);
        QSignalSpy success(&cue, &SleepSoundCue::successAccepted);
        QSignalSpy play(&cue, &SleepSoundCue::playbackRequested);
        cue.beginAttempt();
        cue.complete(false, true);
        cue.complete(true, false);
        QCOMPARE(success.count(), 0);
        cue.complete(true, true);
        cue.complete(true, true);
        QCOMPARE(success.count(), 1);
        QCOMPARE(play.count(), 0);
        cue.setMuted(false); // No delayed sound when the user unmutes.
        QTRY_VERIFY_WITH_TIMEOUT(cue.status() == QSoundEffect::Ready || cue.status() == QSoundEffect::Error, 5000);
        QCOMPARE(play.count(), 0);
        cue.setVolume(0);
        cue.beginAttempt(); cue.complete(true, true);
        QCOMPARE(success.count(), 2);
        QCOMPARE(play.count(), 0);
        cue.setVolume(0.000001f); // Exercise real play dispatch at practically silent gain.
        cue.beginAttempt(); cue.complete(true, true); cue.complete(true, true);
        if (cue.status() == QSoundEffect::Ready && !QMediaDevices::defaultAudioOutput().isNull()) {
            QCOMPARE(play.count(), 1);
            qInfo() << "Sleep cue ready; exactly one playback request dispatched.";
        } else {
            QCOMPARE(play.count(), 0);
            qInfo() << "No usable output; policy verified without audible playback.";
        }
        cue.finishAttempt();
        const int count = success.count();
        cue.complete(true, true);
        QCOMPARE(success.count(), count);
        cue.setMuted(true);
        const int requestsBeforeError = play.count();
        cue.setSource(QUrl(QStringLiteral("qrc:/missing-sleep-cue.wav")));
        cue.beginAttempt(); cue.complete(true, true);
        QTRY_COMPARE_WITH_TIMEOUT(cue.status(), QSoundEffect::Error, 5000);
        QCOMPARE(play.count(), requestsBeforeError);
    }

    void missingSleepCueDoesNotBlockMorning() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("no-cue.json"));
        prepare(window, 720);
        window.m_sleepSound->setSource(QUrl(QStringLiteral("qrc:/missing-sleep-cue.wav")));
        QSignalSpy play(window.m_sleepSound, &SleepSoundCue::playbackRequested);
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QVERIFY(window.m_sleepSaved);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QCOMPARE(play.count(), 0);
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!window.m_sleepBusy, 3000);
        QVERIFY(window.mapWidget->isEnabled());
    }

    void seasonalAtlasRecipesPreserveGridAndProtectedPixels() {
        MapData base;
        QVERIFY(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &base));
        QCOMPARE(base.tilesets.size(), 10);
        for (Season season : {Season::Spring, Season::Summer, Season::Autumn, Season::Winter}) {
            QVector<Tileset> candidate;
            QVERIFY(SeasonVisuals::build(base.tilesets, SeasonVisuals::profile(season), &candidate));
            int changed = 0;
            for (int i = 0; i < candidate.size(); ++i) {
                const auto &original = base.tilesets[i];
                const auto &replacement = candidate[i];
                QCOMPARE(replacement.firstGid, original.firstGid);
                QCOMPARE(replacement.tileWidth, original.tileWidth);
                QCOMPARE(replacement.tileHeight, original.tileHeight);
                QCOMPARE(replacement.tilesPerRow, original.tilesPerRow);
                const QImage a = original.texture.toImage().convertToFormat(QImage::Format_ARGB32);
                const QImage b = replacement.texture.toImage().convertToFormat(QImage::Format_ARGB32);
                QCOMPARE(a.size(), b.size());
                const QString name = QFileInfo(original.sourcePath).fileName();
                const bool vegetation = SeasonVisuals::vegetationAtlases().contains(name);
                for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x) {
                    const QColor source = a.pixelColor(x, y);
                    QCOMPARE(b.pixelColor(x, y).alpha(), source.alpha());
                    const bool selected = vegetation && source.alpha() > 0 &&
                        source.hsvHue() >= 55 && source.hsvHue() <= 170 && source.hsvSaturationF() >= 0.16;
                    if (season == Season::Spring || !selected) QCOMPARE(a.pixel(x, y), b.pixel(x, y));
                    if (a.pixel(x, y) != b.pixel(x, y)) ++changed;
                }
                qInfo().noquote() << "Season atlas:" << GameCalendar::seasonKey(season)
                    << name << a.size() << "GID" << original.firstGid;
            }
            if (season == Season::Spring) QCOMPARE(changed, 0);
            else QVERIFY(changed > 100);
        }
    }

    void seasonTexturesAreTransactionalAndCacheIsBounded() {
        QTemporaryDir dir;
        MapWidget map;
        QVERIFY(map.loadMap(QStringLiteral(":/map/map6.tmj")));
        map.resize(520, 320);
        auto farm = map.worldSnapshot();
        farm.crops = {{QPoint(5, 5), CropType::Wheat, 0, 3},
                      {QPoint(6, 5), CropType::Potato, 0, 1}};
        farm.cultivatedLayer[5][5] = GameConstants::CultivatedTileGid;
        farm.cultivatedLayer[5][6] = GameConstants::CultivatedTileGid;
        farm.wateredTiles = {QPoint(6, 5)};
        QVERIFY(map.restoreWorld(farm));
        const auto collision = map.collisionLayer();
        QSignalSpy mutations(&map, &MapWidget::worldChanged);
        for (int cycle = 0; cycle < 8; ++cycle)
            for (Season season : {Season::Spring, Season::Summer, Season::Autumn, Season::Winter}) {
                QVERIFY(map.setSeasonVisuals(SeasonVisuals::profile(season)));
                QCOMPARE(map.visualSeason(), season);
                QCOMPARE(map.worldSnapshot().cultivatedLayer, farm.cultivatedLayer);
                QCOMPARE(map.worldSnapshot().wateredTiles, farm.wateredTiles);
                QCOMPARE(map.worldSnapshot().lastCompletedDay, farm.lastCompletedDay);
                QCOMPARE(map.worldSnapshot().crops.size(), 2);
                QCOMPARE(map.worldSnapshot().crops[0].growthDays + map.worldSnapshot().crops[1].growthDays, 4);
                QCOMPARE(map.collisionLayer(), collision);
            }
        QCOMPARE(map.seasonTextureBuildCount(), 4);
        QCOMPARE(map.cachedSeasonCount(), 4);
        QCOMPARE(mutations.count(), 0);
        const QImage checkpoint = map.grab().toImage();
        MapData original;
        QVERIFY(MapLoader::load(QStringLiteral(":/map/map6.tmj"), &original));
        const auto grass = original.tilesets[0].texture.toImage();
        const auto tree = original.tilesets[5].texture.toImage();
        const QString valid = dir.filePath("grass.png");
        QVERIFY(grass.save(valid));
        const QString wrongSize = dir.filePath("size.png");
        QImage small(16, 16, QImage::Format_ARGB32); small.fill(Qt::red);
        QVERIFY(small.save(wrongSize));
        const QString alphaPath = dir.filePath("alpha.png");
        QImage alpha = grass.convertToFormat(QImage::Format_ARGB32);
        const QColor old = alpha.pixelColor(0, 0);
        alpha.setPixelColor(0, 0, QColor(old.red(), old.green(), old.blue(), 255 - old.alpha()));
        QVERIFY(alpha.save(alphaPath));
        const QString broken = dir.filePath("broken.png");
        QFile file(broken); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("not an image"); file.close();
        for (const QString &bad : {dir.filePath("missing.png"), broken, wrongSize, alphaPath}) {
            auto recipe = SeasonVisuals::profile(Season::Autumn);
            recipe.textureOverrides.insert(QStringLiteral("Grass.tsx"), bad);
            QString error;
            QVERIFY(!map.setSeasonVisuals(recipe, &error));
            QVERIFY(!error.isEmpty());
            QCOMPARE(map.grab().toImage(), checkpoint);
            QCOMPARE(map.visualSeason(), Season::Winter);
            QCOMPARE(map.seasonTextureBuildCount(), 4);
        }
        auto transaction = SeasonVisuals::profile(Season::Autumn);
        auto unknown = transaction;
        unknown.textureOverrides.insert(QStringLiteral("unknown.tsx"), valid);
        QVERIFY(!map.setSeasonVisuals(unknown));
        QCOMPARE(map.grab().toImage(), checkpoint);
        transaction.textureOverrides.insert(QStringLiteral("Grass.tsx"), valid);
        const QString repairPath = dir.filePath("tree.png");
        transaction.textureOverrides.insert(QStringLiteral("Trees, stumps and bushes.tsx"), repairPath);
        QVERIFY(!map.setSeasonVisuals(transaction));
        QCOMPARE(map.grab().toImage(), checkpoint); // Earlier valid replacement not partially committed.
        QVERIFY(tree.save(repairPath));
        QVERIFY(map.setSeasonVisuals(transaction)); // Failed loads are not cached; retry can recover.
        QVERIFY(map.cachedSeasonCount() <= 4);
        QCOMPARE(map.worldSnapshot().cultivatedLayer, farm.cultivatedLayer);
        QCOMPARE(map.worldSnapshot().wateredTiles, farm.wateredTiles);
        QCOMPARE(mutations.count(), 0);
    }

    void seasonWorldScreenshotsAndHudRemainReadable() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("screenshots.json"));
        prepare(window, 720);
        window.resize(1040, 640);
        window.show();
        QTest::qWait(50);
        QDir().mkpath(QStringLiteral("ui-captures"));
        QList<QImage> scenes;
        for (int index = 0; index < 4; ++index) {
            const Season season = static_cast<Season>(index);
            const int day = 7 * index;
            // Fixture dates are already settled. This test isolates rendering;
            // seasonBoundarySleepAndReload exercises real day settlement.
            auto world = window.mapWidget->worldSnapshot();
            world.lastCompletedDay = day - 1;
            QVERIFY(window.mapWidget->restoreWorld(world));
            window.m_lastProcessedDay = day;
            window.m_gameClock.setElapsedGameMinutes(day * 1440LL + 720);
            window.m_lastObservedGameMinutes = window.m_gameClock.elapsedGameMinutes();
            window.updateTime();
            scenes.append(window.m_gameCanvas->grab().toImage());
            QVERIFY(window.grab().save(QStringLiteral("ui-captures/season-%1-noon.png").arg(GameCalendar::seasonKey(season))));
            window.m_gameClock.setElapsedGameMinutes(day * 1440LL + 1020);
            window.updateTime();
            QVERIFY(window.grab().save(QStringLiteral("ui-captures/season-%1-dusk.png").arg(GameCalendar::seasonKey(season))));
            window.m_gameClock.setElapsedGameMinutes(day * 1440LL + 1380);
            window.updateTime();
            QVERIFY(window.grab().save(QStringLiteral("ui-captures/season-%1-night.png").arg(GameCalendar::seasonKey(season))));
        }
        for (int a = 0; a < scenes.size(); ++a) for (int b = a + 1; b < scenes.size(); ++b) {
            QVERIFY(scenes[a].copy(QRect(350, 110, 100, 80)) != scenes[b].copy(QRect(350, 110, 100, 80)));
            // Transparent corners reveal different terrain; the opaque toolbar
            // interior itself must retain exactly the same colors.
            const QRect bar = window.itemBar->geometry().adjusted(12, 12, -12, -12);
            QCOMPARE(scenes[a].copy(bar), scenes[b].copy(bar));
        }
        QCOMPARE(window.mapWidget->seasonTextureBuildCount(), 4);
        window.toggleInventory();
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/season-winter-backpack.png")));
        QVERIFY(window.m_inventoryPanel->isVisible());
        QCOMPARE(window.m_economy.coins(), 0);
    }

    void seasonBoundarySleepAndReload_data() {
        QTest::addColumn<int>("daysPerSeason");
        QTest::addColumn<int>("lastDay");
        QTest::addColumn<QSize>("size");
        QTest::newRow("spring-summer-small") << 7 << 6 << QSize(520, 320);
        QTest::newRow("summer-autumn-double") << 7 << 13 << QSize(1040, 640);
        QTest::newRow("autumn-winter-wide") << 7 << 20 << QSize(1920, 1080);
        QTest::newRow("winter-year2") << 7 << 27 << QSize(1040, 640);
        QTest::newRow("configured-28days") << 28 << 111 << QSize(1040, 640);
    }

    void seasonBoundarySleepAndReload() {
        QFETCH(int, daysPerSeason);
        QFETCH(int, lastDay);
        QFETCH(QSize, size);
        QTemporaryDir dir;
        const QString path = dir.filePath("calendar.json");
        MainWindow window(nullptr, path, {daysPerSeason});
        prepare(window, lastDay * 1440LL + 1320);
        auto world = window.mapWidget->worldSnapshot();
        world.lastCompletedDay = lastDay - 1;
        world.crops[0].plantedDay = lastDay;
        QVERIFY(window.mapWidget->restoreWorld(world));
        window.m_lastProcessedDay = lastDay;
        window.updateTime();
        QVERIFY(window.saveGame());
        const auto oldDate = *GameCalendar::forDay(lastDay, {daysPerSeason});
        const int originalMapWidth = window.mapWidget->mapWidth();
        QCOMPARE(window.m_displayedDate.season, oldDate.season);
        QSignalSpy changes(&window, &MainWindow::seasonChanged);
        window.resize(size);
        window.show();
        QTest::qWait(50);
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        const auto date = *GameCalendar::forDay(lastDay + 1, {daysPerSeason});
        QCOMPARE(window.m_displayedDate.year, date.year);
        QCOMPARE(window.m_displayedDate.season, date.season);
        QCOMPARE(window.m_displayedDate.dayOfSeason, 1);
        QCOMPARE(changes.count(), 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.size(), 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QVERIFY(window.mapWidget->worldSnapshot().wateredTiles.isEmpty());
        QCOMPARE(window.mapWidget->mapWidth(), originalMapWidth);
        QCOMPARE(window.m_dayNightOverlay->property("seasonId").toString(), GameCalendar::seasonKey(date.season));
        QVERIFY(!window.m_dayNightOverlay->property("seasonVisualFallback").toBool());
        QCOMPARE(window.mapWidget->visualSeason(), date.season);
        const QString fullDate = CalendarPresentation::dateText(date, window.m_calendarFont.chinese);
        QCOMPARE(window.dayLabel->toolTip(), fullDate);
        QVERIFY(window.m_sleepOverlay->morningDateText().contains(fullDate));
        QVERIFY(!window.m_sleepOverlay->dateTransitionText().isEmpty());
        const QFontMetrics metrics(window.dayLabel->font());
        for (const auto &line : window.dayLabel->text().split('\n'))
            QVERIFY(metrics.horizontalAdvance(line) <= window.dayLabel->width());
        QVERIFY(metrics.lineSpacing() * 2 <= window.dayLabel->height());
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/calendar-%1-%2-sleep.png")
                                   .arg(daysPerSeason).arg(lastDay)));
        window.updateTime();
        QCOMPARE(changes.count(), 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        MainWindow loaded(nullptr, path, {1}); // Saved rules override new-game defaults.
        loaded.m_gameClock.pause();
        QCOMPARE(loaded.m_calendarRules.daysPerSeason, daysPerSeason);
        QCOMPARE(loaded.mapWidget->visualSeason(), date.season);
        QCOMPARE(loaded.m_dayNightOverlay->tint(),
                 DayNightEnvironment::tintAt(loaded.m_gameClock.elapsedGameMinutes(), true, date.season));
        QCOMPARE(loaded.dayLabel->toolTip(), fullDate);
        QCOMPARE(loaded.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        loaded.resize(size);
        loaded.show();
        QTest::qWait(50);
        QVERIFY(loaded.grab().save(QStringLiteral("ui-captures/calendar-%1-%2-hud.png")
                                   .arg(daysPerSeason).arg(lastDay)));
        loaded.close();
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!window.m_sleepBusy, 3000);
        window.m_gameClock.pause();
        window.m_gameClock.setElapsedGameMinutes((lastDay + 1) * 1440LL + 120);
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), (lastDay + 1) * 1440LL + 360);
        QCOMPARE(changes.count(), 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void naturalSeasonBoundaryWhileBackpackOpen() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("natural.json"));
        prepare(window, 6 * 1440LL + 1439);
        auto world = window.mapWidget->worldSnapshot();
        world.lastCompletedDay = 5;
        world.crops[0].plantedDay = 6;
        QVERIFY(window.mapWidget->restoreWorld(world));
        window.m_lastProcessedDay = 6;
        window.updateTime();
        window.toggleInventory();
        QSignalSpy changes(&window, &MainWindow::seasonChanged);
        window.m_gameClock.setElapsedGameMinutes(7 * 1440LL);
        window.updateTime();
        window.updateTime();
        QCOMPARE(changes.count(), 1);
        QCOMPARE(window.m_displayedDate.season, Season::Summer);
        QVERIFY(!window.mapWidget->isEnabled());
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        window.toggleInventory();
        QVERIFY(window.mapWidget->isEnabled());
    }

    void calendarFontAndFallbackText() {
        const auto font = CalendarPresentation::chooseFont();
        qInfo().noquote() << "Calendar font:" << font.font.family()
                          << "Chinese:" << font.chinese;
        const CalendarDate date{2, Season::Winter, 28};
        QCOMPARE(CalendarPresentation::dateText(date, false), QStringLiteral("Y2 WINTER DAY 28"));
        QCOMPARE(CalendarPresentation::dateText(date, true), QStringLiteral("第2年 冬季 第28天"));
        if (font.chinese) {
            const QRawFont raw = QRawFont::fromFont(font.font);
            for (const QChar c : QStringLiteral("第年春夏秋冬季天"))
                QVERIFY(raw.supportsCharacter(c.unicode()));
        }
        // Render without showing a desktop window. Native-platform runs can
        // verify installed CJK fonts even when offscreen lacks system fonts.
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("font.json"));
        window.m_gameClock.pause();
        window.updateTime();
        const QFontMetrics metrics(window.dayLabel->font());
        for (const auto &line : window.dayLabel->text().split('\n'))
            QVERIFY(metrics.horizontalAdvance(line) <= window.dayLabel->width());
        QVERIFY(metrics.lineSpacing() * 2 <= window.dayLabel->height());
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.m_gameCanvas->grab().save(font.chinese
            ? QStringLiteral("ui-captures/calendar-chinese.png")
            : QStringLiteral("ui-captures/calendar-english.png")));
        QVERIFY(window.m_gameCanvas->rect().contains(window.timeLabel->geometry()));
        window.m_gameClock.setElapsedGameMinutes(720);
        window.character->setInitialPosition(QPoint(256, 176));
        window.updateTime();
        window.requestSleep();
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Confirming);
        qInfo() << "Confirmation render device scale:" << window.m_gameCanvas->devicePixelRatioF();
        QVERIFY(window.m_gameCanvas->grab().save(QStringLiteral("ui-captures/sleep-confirmation-native.png")));
        window.cancelSleepConfirmation();
        prepare(window, 27 * 1440LL + 1320);
        auto world = window.mapWidget->worldSnapshot();
        world.lastCompletedDay = 26;
        world.crops[0].plantedDay = 27;
        QVERIFY(window.mapWidget->restoreWorld(world));
        window.m_lastProcessedDay = 27;
        window.updateTime();
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QVERIFY(window.m_gameCanvas->grab().save(font.chinese
            ? QStringLiteral("ui-captures/calendar-chinese-sleep.png")
            : QStringLiteral("ui-captures/calendar-english-sleep.png")));
    }

    void multiSeasonAdvanceAndFailedBoundaryStayCoherent() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("multi.json"));
        prepare(window, 6 * 1440LL + 1439);
        auto world = window.mapWidget->worldSnapshot();
        world.lastCompletedDay = 5;
        world.crops[0].plantedDay = 6;
        QVERIFY(window.mapWidget->restoreWorld(world));
        window.m_lastProcessedDay = 6;
        window.updateTime();
        QVERIFY(window.saveGame());
        QSignalSpy changes(&window, &MainWindow::seasonChanged);
        window.m_gameClock.setElapsedGameMinutes(29 * 1440LL + 540);
        window.m_lastObservedGameMinutes = window.m_gameClock.elapsedGameMinutes(); // Explicit settlement fixture.
        const auto result = window.processDayTransitions();
        QCOMPARE(result.cropsAdvanced, 1);
        QCOMPARE(result.cropsStalled, 22);
        window.updateTime();
        QCOMPARE(window.m_displayedDate.year, 2LL);
        QCOMPARE(window.m_displayedDate.season, Season::Spring);
        QCOMPARE(window.m_displayedDate.dayOfSeason, 2);
        QCOMPARE(window.m_lastProcessedDay, 29);
        QCOMPARE(window.mapWidget->worldSnapshot().lastCompletedDay, 28);
        QCOMPARE(changes.count(), 1); // Presentation refresh, not a gameplay event queue.
        window.updateTime();
        QCOMPARE(changes.count(), 1);
        const QString checkpointDate = window.dayLabel->text();
        window.m_lastProcessedDay = 30; // Invalid marker: do not announce a new season.
        window.m_gameClock.setElapsedGameMinutes(35 * 1440LL);
        window.m_lastObservedGameMinutes = window.m_gameClock.elapsedGameMinutes();
        window.updateTime();
        QVERIFY(window.m_dayAdvanceFailed);
        QCOMPARE(changes.count(), 1);
        QCOMPARE(window.dayLabel->text(), checkpointDate);
        QVERIFY(!window.saveGame());
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.lastProcessedDay, 29);
    }

    void dayNightLayersInputAndReload() {
        QTemporaryDir dir;
        const QString savePath = dir.filePath("environment.json");
        MainWindow window(nullptr, savePath);
        prepare(window, 720);
        window.resize(1040, 640);
        window.show();
        QTest::qWait(50); // Let the viewport's deferred resize/scale settle.
        window.updateGameScale();
        window.updateTime();
        const auto worldBefore = window.mapWidget->worldSnapshot();
        const auto economyBefore = window.m_economy.state();
        const QRect playerRect(window.character->mapTo(window.m_gameCanvas, QPoint()),
                               window.character->size());
        const QImage day = window.m_gameCanvas->grab().toImage();
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/environment-day.png")));
        window.m_gameClock.setElapsedGameMinutes(1320);
        window.updateTime();
        const QImage night = window.m_gameCanvas->grab().toImage();
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/environment-night.png")));
        // World/player darken. Opaque UI pixels remain unchanged; translucent
        // HUD backgrounds intentionally reveal the newly tinted world below.
        QVERIFY(day.copy(playerRect) != night.copy(playerRect));
        QVERIFY(day.copy(QRect(380, 120, 50, 35)) != night.copy(QRect(380, 120, 50, 35)));
        int whiteTextPixels = 0;
        const QRect hudRect = window.m_coinsLabel->geometry();
        for (int y = hudRect.top(); y <= hudRect.bottom(); ++y)
            for (int x = hudRect.left(); x <= hudRect.right(); ++x)
                if (day.pixelColor(x, y) == QColor(Qt::white)) {
                    QCOMPARE(night.pixelColor(x, y), QColor(Qt::white));
                    ++whiteTextPixels;
                }
        QVERIFY(whiteTextPixels > 10);
        int unchangedToolPixels = 0;
        const QRect toolRect = window.itemBar->geometry();
        for (int y = toolRect.top(); y <= toolRect.bottom(); ++y)
            for (int x = toolRect.left(); x <= toolRect.right(); ++x)
                if (day.pixel(x, y) == night.pixel(x, y)) ++unchangedToolPixels;
        QVERIFY(unchangedToolPixels > toolRect.width() * toolRect.height() / 2);
        QVERIFY(window.m_dayNightOverlay->testAttribute(Qt::WA_TransparentForMouseEvents));
        QSignalSpy clicks(window.mapWidget, &MapWidget::mapClicked);
        const QPoint viewportPoint = window.m_gameView->mapFromScene(QPointF(380, 120));
        QTest::mouseClick(window.m_gameView->viewport(), Qt::LeftButton, Qt::NoModifier, viewportPoint);
        QCOMPARE(clicks.count(), 1);
        const QPoint beforeMove = window.character->absolutePosition();
        QTest::keyPress(window.character, Qt::Key_D);
        QTest::qWait(180);
        QTest::keyRelease(window.character, Qt::Key_D);
        QVERIFY(window.character->absolutePosition() != beforeMove);
        window.toggleInventory();
        const QImage bagNight = window.m_inventoryPanel->grab().toImage();
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/environment-night-backpack.png")));
        window.m_gameClock.setElapsedGameMinutes(720);
        window.updateTime();
        QCOMPARE(window.m_inventoryPanel->grab().toImage(), bagNight);
        window.toggleInventory();
        window.m_gameClock.setElapsedGameMinutes(1140);
        window.updateTime();
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/environment-dusk.png")));
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, worldBefore.crops.first().growthDays);
        QCOMPARE(window.mapWidget->worldSnapshot().wateredTiles, worldBefore.wateredTiles);
        QCOMPARE(window.m_economy.state().coins, economyBefore.coins);
        QCOMPARE(window.m_economy.state().inventoryCounts, economyBefore.inventoryCounts);
        QCOMPARE(window.m_economy.state().seedCounts, economyBefore.seedCounts);
        window.m_gameClock.setElapsedGameMinutes(1320);
        QVERIFY(window.saveGame());
        MainWindow loaded(nullptr, savePath);
        loaded.m_gameClock.pause();
        QCOMPARE(loaded.m_dayNightOverlay->tint(), DayNightEnvironment::tintAt(1320));
        loaded.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(loaded.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(loaded.m_dayNightOverlay->tint(), DayNightEnvironment::tintAt(1800));
    }

    void midnightAndMultiDaySaveStayConsistent() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 23 * 60 + 59);
        QVERIFY(window.saveGame());
        window.m_gameClock.setElapsedGameMinutes(3 * 1440 + 540);
        window.m_lastObservedGameMinutes = window.m_gameClock.elapsedGameMinutes(); // Explicit settlement fixture.
        const auto result = window.processDayTransitions();
        QCOMPARE(result.cropsAdvanced, 1);
        QCOMPARE(result.cropsStalled, 2);
        QCOMPARE(window.m_lastProcessedDay, 3);
        QCOMPARE(window.mapWidget->worldSnapshot().lastCompletedDay, 2);
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.gameMinutes, 3LL * 1440 + 540);
        QCOMPARE(saved.lastProcessedDay, 3);
        QCOMPARE(saved.world.lastCompletedDay, 2);
        QCOMPARE(saved.world.crops.first().growthDays, 1);
        QCOMPARE(window.processDayTransitions().cropsAdvanced, 0);
        MainWindow restored(nullptr, window.m_savePath);
        restored.m_gameClock.pause();
        QCOMPARE(restored.m_lastProcessedDay, 3);
        QCOMPARE(restored.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void saveGuardPreservesPreviousCheckpoint() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 23 * 60 + 59);
        QVERIFY(window.saveGame());
        QFile file(window.m_savePath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray checkpoint = file.readAll();
        file.close();
        // Explicitly exercise the guard used during synchronous day settlement.
        // advanceDay does not emit worldChanged; no imaginary signal is assumed.
        window.m_processingDays = true;
        window.m_gameClock.setElapsedGameMinutes(1440);
        QVERIFY(!window.saveGame());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), checkpoint);
        file.close();
        window.m_processingDays = false;
        QVERIFY(window.saveGame());
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.lastProcessedDay, 1);
        QCOMPARE(saved.world.lastCompletedDay, 0);
    }

    void midnightWhileBackpackOpenAndReload() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 1439);
        QVERIFY(window.saveGame());
        {
            MainWindow beforeMidnight(nullptr, window.m_savePath);
            beforeMidnight.m_gameClock.pause();
            QCOMPARE(beforeMidnight.mapWidget->worldSnapshot().crops.first().growthDays, 0);
        }
        window.toggleInventory();
        QVERIFY(!window.mapWidget->isEnabled());
        window.m_gameClock.setElapsedGameMinutes(1440);
        window.updateTime();
        window.updateTime();
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QVERIFY(!window.mapWidget->worldSnapshot().wateredTiles.contains(QPoint(5, 5)));
        window.toggleInventory();
        QVERIFY(window.mapWidget->isEnabled());
        MainWindow afterMidnight(nullptr, window.m_savePath);
        afterMidnight.m_gameClock.pause();
        QCOMPARE(afterMidnight.m_lastProcessedDay, 1);
        QCOMPARE(afterMidnight.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void settlementFailureDoesNotShowSuccessAndCanRetry() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 1320);
        QSignalSpy cues(window.m_sleepSound, &SleepSoundCue::successAccepted);
        QVERIFY(window.saveGame());
        window.sleepUntilMorning();
        window.m_lastProcessedDay = 2; // Inject inconsistent state before checkpoint.
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Failed, 3000);
        QVERIFY(!window.m_sleepSettled);
        QCOMPARE(cues.count(), 0);
        window.resize(1040, 640);
        window.show();
        QTest::qWait(50);
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/settlement-failure.png")));
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1320LL);
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.gameMinutes, 1320LL);
        window.m_lastProcessedDay = 0; // Repair dependency, then retry the day.
        window.m_sleepOverlay->retrySaveRequested();
        QCOMPARE(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning);
        QVERIFY(window.m_sleepSaved);
        QCOMPARE(cues.count(), 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void closingAtEachSleepPhaseRestoresOneConsistentDay_data() {
        QTest::addColumn<bool>("afterSettlement");
        QTest::newRow("during-fade") << false;
        QTest::newRow("morning-summary") << true;
    }
    void closingAtEachSleepPhaseRestoresOneConsistentDay() {
        QFETCH(bool, afterSettlement);
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 1320);
        window.show();
        window.sleepUntilMorning();
        if (afterSettlement)
            QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        window.close();
        MainWindow restored(nullptr, window.m_savePath);
        restored.m_gameClock.pause();
        QCOMPARE(restored.currentGameDay(), afterSettlement ? 1 : 0);
        QCOMPARE(restored.mapWidget->worldSnapshot().crops.first().growthDays, afterSettlement ? 1 : 0);
    }

    void sleepAnimationSettlesOnceAndBlocksInput() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 22 * 60);
        window.show();
        window.sleepUntilMorning();
        QVERIFY(window.m_sleepBusy);
        QVERIFY(window.m_gameClock.isPaused());
        QVERIFY(!window.mapWidget->isEnabled());
        QCOMPARE(window.currentGameDay(), 0); // Only settle at opaque checkpoint.
        window.toggleInventory();
        QVERIFY(window.m_inventoryPanel->isHidden());
        window.sleepUntilMorning(); // Reentrant/duplicate request is ignored.
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.currentGameDay(), 1);
        QCOMPARE(window.currentGameHour(), 6);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QVERIFY(window.m_sleepSaved);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        window.sleepUntilMorning();
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QTest::keyClick(window.m_sleepOverlay, Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!window.m_sleepBusy, 3000);
        QVERIFY(window.mapWidget->isEnabled());
        QVERIFY(!window.m_gameClock.isPaused());
        window.m_gameClock.pause();
        MainWindow restored(nullptr, window.m_savePath);
        restored.m_gameClock.pause();
        QCOMPARE(restored.currentGameDay(), 1);
        QCOMPARE(restored.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void earlyMorningDoesNotRepeatMidnight() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 1440 + 120);
        window.updateTime();
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.currentGameDay(), 1);
        QCOMPARE(window.currentGameHour(), 6);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
    }

    void daytimeSleepSucceedsAndCorruptMarkerRejects() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 12 * 60);
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.currentGameDay(), 1);
        QCOMPARE(window.currentGameHour(), 6);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        QFile file(window.m_savePath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto checkpoint = file.readAll();
        file.close();
        window.m_gameClock.pause();
        window.m_lastProcessedDay = 2;
        QVERIFY(!window.saveGame());
        QVERIFY(window.m_dayAdvanceFailed);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), checkpoint);
    }

    void failedSaveCanRetryWithoutGrowingAgain() {
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.path()); // Directory cannot be a save file.
        // A normal fresh world is used; the intentional I/O failure is isolated.
        prepare(window, 22 * 60);
        QSignalSpy cues(window.m_sleepSound, &SleepSoundCue::successAccepted);
        window.m_saveEnabled = true;
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QVERIFY(!window.m_sleepSaved);
        QCOMPARE(cues.count(), 0);
        window.resize(1040, 640);
        window.show();
        QTest::qWait(50);
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/save-failure.png")));
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        window.m_savePath = dir.filePath("recovered.json");
        window.m_sleepOverlay->retrySaveRequested();
        QVERIFY(window.m_sleepSaved);
        QCOMPARE(cues.count(), 1);
        window.m_sleepOverlay->retrySaveRequested();
        QCOMPARE(cues.count(), 1);
        QCOMPARE(window.mapWidget->worldSnapshot().crops.first().growthDays, 1);
        GameSaveData saved;
        QVERIFY(SaveManager::load(window.m_savePath, &saved));
        QCOMPARE(saved.world.crops.first().growthDays, 1);
    }

    void renderedLayoutAndViewportMouseWakeUp_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(520, 320);
        QTest::newRow("double") << QSize(1040, 640);
        QTest::newRow("wide") << QSize(1920, 1080);
    }

    void renderedLayoutAndViewportMouseWakeUp() {
        QFETCH(QSize, size);
        QTemporaryDir dir;
        MainWindow window(nullptr, dir.filePath("save.json"));
        prepare(window, 1320);
        window.resize(size);
        window.show();
        QTest::qWait(50);
        window.updateGameScale();
        window.updateTime();
        QCOMPARE(window.m_dayNightOverlay->geometry(), window.mapWidget->rect());
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/environment-night-%1.png").arg(size.width())));
        window.sleepUntilMorning();
        QTRY_COMPARE_WITH_TIMEOUT(window.m_sleepOverlay->phase(), SleepOverlay::Phase::Morning, 3000);
        QCOMPARE(window.m_dayNightOverlay->tint(), DayNightEnvironment::tintAt(1800));
        // Verify the real widgets, not a separately drawn design mockup.
        QDir().mkpath(QStringLiteral("ui-captures"));
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/morning-%1.png").arg(size.width())));
        QTest::qWait(1100);
        QCOMPARE(window.m_gameClock.elapsedGameMinutes(), 1800LL);
        QPushButton *wake = nullptr;
        for (auto *button : window.m_sleepOverlay->findChildren<QPushButton *>())
            if (button->text().contains(QStringLiteral("WAKE UP"))) wake = button;
        QVERIFY(wake);
        const QPoint logical = wake->mapTo(window.m_gameCanvas, wake->rect().center());
        const QPoint viewportPoint = window.m_gameView->mapFromScene(QPointF(logical));
        // Exercise graphics-view input coordinate mapping at every tested size.
        QTest::mouseClick(window.m_gameView->viewport(), Qt::LeftButton,
                          Qt::NoModifier, viewportPoint);
        QTRY_VERIFY_WITH_TIMEOUT(!window.m_sleepBusy, 3000);
        QVERIFY(window.mapWidget->isEnabled());
        QVERIFY(!window.m_gameClock.isPaused());
        QVERIFY(window.grab().save(QStringLiteral("ui-captures/awake-%1.png").arg(size.width())));
        const QPoint previous = window.character->absolutePosition();
        QTest::keyPress(window.character, Qt::Key_D);
        QTest::qWait(180);
        QTest::keyRelease(window.character, Qt::Key_D);
        QVERIFY(window.character->absolutePosition() != previous);
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--preview-sleep")) ||
        app.arguments().contains(QStringLiteral("--preview-passout")) ||
        app.arguments().contains(QStringLiteral("--preview-confirm")) ||
        app.arguments().contains(QStringLiteral("--preview-calendar")) ||
        app.arguments().contains(QStringLiteral("--preview-daynight")))
        return SleepFlowTests::previewSleep();
    SleepFlowTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "sleep_ui_tests.moc"
