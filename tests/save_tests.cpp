#include "storage/savemanager.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

namespace {

MapData testMap() {
    MapData map;
    map.mapWidth = 2;
    map.mapHeight = 1;
    map.cultivatedLayer = {{GameConstants::CultivatedTileGid, 0}};
    return map;
}

GameSaveData newFarm() {
    WorldState world;
    world.initialize(testMap());
    world.plantSeed({0, 0}, CropType::Wheat, 0);
    world.fertilize({0, 0});

    GameSaveData data;
    data.gameMinutes = 360;
    data.world = world.snapshot();
    data.economy = EconomyService().state();
    data.economy.fertilizerCount = 7;
    return data;
}

QByteArray readFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
           file.write(bytes) == bytes.size() && file.flush();
}

} // namespace

class SaveTests : public QObject {
    Q_OBJECT

private slots:
    void fertilizerRoundTrip();
    void missingV2StockMigratesWithoutWritingLegacy();
    void migrateV3AndRoundTripStorage();
    void newestGenerationWithEitherCopyTakesPriority();
    void invalidStorageIsRejected_data();
    void invalidStorageIsRejected();
    void invalidV3StockRejected_data();
    void invalidV3StockRejected();
    void backupSurvivesUnreadablePrimary();
    void backupSurvivesRuntimeRejectedPrimary_data();
    void backupSurvivesRuntimeRejectedPrimary();
    void invalidCopiesArePreserved();
    void rejectedSnapshotAndFailedBackupLeavePrimaryAlone();
};

void SaveTests::fertilizerRoundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    const GameSaveData original = newFarm();
    QVERIFY(SaveManager::save(path, original));

    GameSaveData loaded;
    QVERIFY(SaveManager::load(path, &loaded));
    QCOMPARE(loaded.economy.fertilizerCount, 7);
    WorldState world;
    world.initialize(testMap());
    QVERIFY(world.restore(loaded.world));
    QVERIFY(world.isFertilized({0, 0}));
    QVERIFY(world.hasPendingFertilizerBonus({0, 0}));
    QVERIFY(world.waterTile({0, 0}));
    QCOMPARE(world.advanceDay(0).fertilizerBonusesUsed, 1);
    loaded.lastProcessedDay = 1;
    loaded.gameMinutes += 1440;
    loaded.world = world.snapshot();
    QVERIFY(SaveManager::save(path, loaded));
    QVERIFY(SaveManager::load(path, &loaded));
    QVERIFY(world.restore(loaded.world));
    QVERIFY(world.isFertilized({0, 0}));
    QVERIFY(!world.hasPendingFertilizerBonus({0, 0}));
    QCOMPARE(world.advanceDay(0).fertilizerBonusesUsed, 0);
}

void SaveTests::missingV2StockMigratesWithoutWritingLegacy() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QFileInfo current(SaveManager::defaultSavePath());
    const QFileInfo legacy(SaveManager::legacySavePath());
    QCOMPARE(current.fileName(), QStringLiteral("farm-v4.json"));
    QCOMPARE(QFileInfo(SaveManager::previousSavePath()).fileName(), QStringLiteral("farm-v3.json"));
    QCOMPARE(legacy.fileName(), QStringLiteral("savegame.json"));
    QCOMPARE(current.absolutePath(), legacy.absolutePath());
    const QString currentPath = dir.filePath(current.fileName());
    const QString legacyPath = dir.filePath(legacy.fileName());

    const QString fixture = dir.filePath("fixture.json");
    QVERIFY(SaveManager::save(fixture, newFarm()));
    auto v2 = QJsonDocument::fromJson(readFile(fixture)).object();
    v2.insert("schemaVersion", 2);
    auto economy = v2.value("economy").toObject();
    economy.remove("fertilizerCount");
    economy.remove("storage");
    v2.insert("economy", economy);
    const QByteArray previous = QJsonDocument(v2).toJson();
    QVERIFY(writeFile(legacyPath, previous));
    GameSaveData loaded;
    QVERIFY(SaveManager::load(legacyPath, &loaded));
    QCOMPARE(loaded.economy.fertilizerCount, 0);
    QCOMPARE(loaded.economy.storage.fertilizerCount, 0);
    QVERIFY(loaded.economy.storage.harvestCounts.isEmpty());
    QVERIFY(loaded.world.crops.first().fertilizerBonusPending);
    QVERIFY(SaveManager::save(currentPath, loaded));
    QCOMPARE(readFile(legacyPath), previous);
    QVERIFY(SaveManager::load(currentPath, &loaded));
    QCOMPARE(loaded.economy.fertilizerCount, 0);

    v2.insert("schemaVersion", 1);
    const QByteArray v1 = QJsonDocument(v2).toJson();
    QVERIFY(writeFile(legacyPath, v1));
    QVERIFY(!SaveManager::load(legacyPath, &loaded));
    QVERIFY(SaveManager::save(currentPath, newFarm()));
    QCOMPARE(readFile(legacyPath), v1);
}

void SaveTests::migrateV3AndRoundTripStorage() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString current = dir.filePath("farm-v4.json");
    const QString old = dir.filePath("farm-v3.json");
    const QString fixture = dir.filePath("sample.json");
    QVERIFY(SaveManager::save(fixture, newFarm()));
    auto root = QJsonDocument::fromJson(readFile(fixture)).object();
    root["schemaVersion"] = 3;
    auto economy = root["economy"].toObject();
    economy.remove("storage");
    root["economy"] = economy;
    const QByteArray v3 = QJsonDocument(root).toJson();
    QVERIFY(writeFile(old, v3));
    QVERIFY(writeFile(old + ".bak", v3));
    GameSaveData loaded;
    QVERIFY(SaveManager::load(old, &loaded));
    QCOMPARE(loaded.economy.fertilizerCount, 7);
    QVERIFY(loaded.economy.storage.harvestCounts.isEmpty());
    QVERIFY(loaded.economy.storage.seedCounts.isEmpty());
    QCOMPARE(loaded.economy.storage.fertilizerCount, 0);

    loaded.economy.storage.seedCounts.insert(static_cast<int>(CropType::Wheat), 3);
    loaded.economy.storage.harvestCounts.insert(static_cast<int>(CropType::Carrot), 120);
    loaded.economy.storage.fertilizerCount = 5;
    QVERIFY(SaveManager::save(current, loaded));
    QCOMPARE(readFile(old), v3);
    QCOMPARE(readFile(old + ".bak"), v3);
    GameSaveData restored;
    QVERIFY(SaveManager::load(current, &restored));
    QCOMPARE(restored.economy.storage.harvestCounts, loaded.economy.storage.harvestCounts);
    QCOMPARE(restored.economy.storage.seedCounts, loaded.economy.storage.seedCounts);
    QCOMPARE(restored.economy.storage.fertilizerCount, 5);
    QCOMPARE(restored.economy.inventoryCapacity, 12);
}

void SaveTests::newestGenerationWithEitherCopyTakesPriority() {
    const QString previousName = QCoreApplication::applicationName();
    const auto resetName = qScopeGuard([previousName] { QCoreApplication::setApplicationName(previousName); });
    QCoreApplication::setApplicationName(QStringLiteral("CloverCog-storage-location-test"));
    const QDir parent = QFileInfo(QFileInfo(SaveManager::defaultSavePath()).absolutePath()).dir();
    QVERIFY(parent.mkpath(QStringLiteral(".")));
    QTemporaryDir dir(parent.filePath(QStringLiteral("CloverCog-storage-test-XXXXXX")));
    QVERIFY(dir.isValid());
    QCoreApplication::setApplicationName(QFileInfo(dir.path()).fileName());
    const QString current = SaveManager::defaultSavePath();
    const QString previous = SaveManager::previousSavePath();
    const QString legacy = SaveManager::legacySavePath();
    for (const QString &path : {current, previous, legacy})
        QCOMPARE(QFileInfo(path).absolutePath(), dir.path());
    QVERIFY(SaveManager::preferredSavePath().isEmpty());
    QVERIFY(writeFile(legacy + ".bak", "validity checked after selection"));
    QCOMPARE(SaveManager::preferredSavePath(), legacy);
    QVERIFY(writeFile(previous, "v3"));
    QCOMPARE(SaveManager::preferredSavePath(), previous);
    QVERIFY(writeFile(current + ".bak", "damaged"));
    QCOMPARE(SaveManager::preferredSavePath(), current);
    QVERIFY(writeFile(current, "damaged"));
    QVERIFY(QFile::remove(current + ".bak"));
    QCOMPARE(SaveManager::preferredSavePath(), current);
}

void SaveTests::invalidStorageIsRejected_data() {
    QTest::addColumn<QJsonValue>("badStorage");
    const QJsonObject valid{{"harvest", QJsonObject()}, {"seeds", QJsonObject()}, {"fertilizerCount", 0}};
    QTest::newRow("missing") << QJsonValue(QJsonValue::Undefined);
    QTest::newRow("not-an-object") << QJsonValue("wheat");
    auto bad = valid;
    bad.remove("seeds");
    QTest::newRow("missing-seeds") << QJsonValue(bad);
    bad = valid;
    bad.insert("harvest", QJsonObject{{"unknown", 3}});
    QTest::newRow("unknown-crop") << QJsonValue(bad);
    bad = valid;
    bad.insert("seeds", QJsonObject{{"potato", -1}});
    QTest::newRow("negative") << QJsonValue(bad);
    bad = valid;
    bad.insert("fertilizerCount", 2147483648.0);
    QTest::newRow("overflow") << QJsonValue(bad);
    bad.insert("fertilizerCount", 1.5);
    QTest::newRow("fractional") << QJsonValue(bad);
    bad = valid;
    bad.insert("unknown-category", QJsonObject{{"wheat", 5}});
    QTest::newRow("unknown-category") << QJsonValue(bad);
}

void SaveTests::invalidStorageIsRejected() {
    QFETCH(QJsonValue, badStorage);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm-v4.json");
    GameSaveData original = newFarm();
    original.economy.storage.fertilizerCount = 43;
    QVERIFY(SaveManager::save(path, original));
    const QByteArray valid = readFile(path + ".bak");
    auto broken = QJsonDocument::fromJson(valid).object();
    auto economy = broken["economy"].toObject();
    economy.insert("storage", badStorage);
    broken["economy"] = economy;
    const auto invalid = QJsonDocument(broken).toJson();
    QVERIFY(writeFile(path, invalid));
    auto loaded = original;
    QVERIFY(!SaveManager::load(path, &loaded));
    QCOMPARE(loaded.economy.storage.fertilizerCount, 43);
    QVERIFY(SaveManager::load(path + ".bak", &loaded));
    QVERIFY(SaveManager::save(path, loaded));
    QCOMPARE(readFile(path + ".bak"), valid);
    QVERIFY(writeFile(path, invalid));
    QVERIFY(writeFile(path + ".bak", invalid));
    QVERIFY(!SaveManager::save(path, loaded));
    QCOMPARE(readFile(path), invalid);
    QCOMPARE(readFile(path + ".bak"), invalid);
}

void SaveTests::invalidV3StockRejected_data() {
    QTest::addColumn<QJsonValue>("stock");
    QTest::newRow("missing") << QJsonValue(QJsonValue::Undefined);
    QTest::newRow("negative") << QJsonValue(-1);
    QTest::newRow("fractional") << QJsonValue(0.5);
    QTest::newRow("overflow") << QJsonValue(2147483648.0);
    QTest::newRow("string") << QJsonValue(QStringLiteral("1"));
}

void SaveTests::invalidV3StockRejected() {
    QFETCH(QJsonValue, stock);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    QVERIFY(SaveManager::save(path, newFarm()));
    auto root = QJsonDocument::fromJson(readFile(path)).object();
    root.insert("schemaVersion", 3);
    auto economy = root.value("economy").toObject();
    economy.remove("storage");
    economy.insert("fertilizerCount", stock);
    root.insert("economy", economy);
    QVERIFY(writeFile(path, QJsonDocument(root).toJson()));
    GameSaveData unchanged = newFarm();
    QVERIFY(!SaveManager::load(path, &unchanged));
    QCOMPARE(unchanged.economy.fertilizerCount, 7);
}

void SaveTests::backupSurvivesUnreadablePrimary() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    const QString backup = path + ".bak";
    auto data = newFarm();
    QVERIFY(SaveManager::save(path, data));
    const QByteArray original = readFile(path);
    QVERIFY(!original.isEmpty());
    data.economy.coins = 15;
    QVERIFY(SaveManager::save(path, data));
    QCOMPARE(readFile(backup), original);
    auto broken = QJsonDocument::fromJson(readFile(path)).object();
    auto economy = broken.value("economy").toObject();
    economy.insert("inventory", QJsonObject{{"wheat", 13}});
    broken.insert("economy", economy);
    QVERIFY(writeFile(path, QJsonDocument(broken).toJson()));
    QVERIFY(!SaveManager::load(path, &data));
    QVERIFY(SaveManager::load(backup, &data));
    data.economy.coins = 5;
    QVERIFY(SaveManager::save(path, data));
    QCOMPARE(readFile(backup), original);
    QVERIFY(SaveManager::load(path, &data));
    QCOMPARE(data.economy.coins, 5);
}

void SaveTests::backupSurvivesRuntimeRejectedPrimary_data() {
    QTest::addColumn<bool>("corruptPosition");
    QTest::newRow("outside-map") << true;
    QTest::newRow("wrong-capacity") << false;
}

void SaveTests::backupSurvivesRuntimeRejectedPrimary() {
    QFETCH(bool, corruptPosition);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    const QString backup = path + ".bak";
    const auto compatible = [](const GameSaveData &data) {
        return data.playerPosition.x() < 2 * 16 && data.economy.inventoryCapacity == 12;
    };
    auto data = newFarm();
    QVERIFY(SaveManager::save(path, data, nullptr, compatible));
    const QByteArray original = readFile(path);
    auto invalid = data;
    if (corruptPosition) {
        invalid.playerPosition.setX(1000);
    } else {
        invalid.economy.inventoryCapacity = 99;
    }
    // A rejected live snapshot must not alter either copy.
    QVERIFY(!SaveManager::save(path, invalid, nullptr, compatible));
    QCOMPARE(readFile(path), original);
    QCOMPARE(readFile(backup), original);

    // Change the primary on disk after startup, leaving the playable backup untouched.
    const QString replacement = dir.filePath("replacement.json");
    QVERIFY(SaveManager::save(replacement, invalid));
    QVERIFY(writeFile(path, readFile(replacement)));
    data.economy.coins = 6;
    QVERIFY(SaveManager::save(path, data, nullptr, compatible));
    QCOMPARE(readFile(backup), original);
    QVERIFY(SaveManager::load(path, &data));
    QCOMPARE(data.economy.coins, 6);
    QCOMPARE(data.playerPosition, QPoint(0, 0));
    QCOMPARE(data.economy.inventoryCapacity, 12);

    // If both files are unplayable, preserve them and report the failure.
    const QByteArray rejected = readFile(replacement);
    QVERIFY(writeFile(path, rejected));
    QVERIFY(writeFile(backup, rejected));
    QVERIFY(!SaveManager::save(path, data, nullptr, compatible));
    QCOMPARE(readFile(path), rejected);
    QCOMPARE(readFile(backup), rejected);
}

void SaveTests::invalidCopiesArePreserved() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    const QString backup = path + ".bak";
    QVERIFY(writeFile(path, "{"));
    QVERIFY(writeFile(backup, R"({"schemaVersion":1})"));
    QVERIFY(!SaveManager::save(path, newFarm()));
    QCOMPARE(readFile(path), QByteArray("{"));
    QCOMPARE(readFile(backup), QByteArray(R"({"schemaVersion":1})"));
}

void SaveTests::rejectedSnapshotAndFailedBackupLeavePrimaryAlone() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    const QString backup = path + ".bak";
    auto data = newFarm();
    QVERIFY(SaveManager::save(path, data));
    const QByteArray original = readFile(path);
    data.economy.fertilizerCount = -1;
    QVERIFY(!SaveManager::save(path, data));
    QCOMPARE(readFile(path), original);
    QCOMPARE(readFile(backup), original);

    QVERIFY(QFile::remove(backup));
    QVERIFY(QDir().mkdir(backup)); // Cannot atomically replace a directory with a file.
    data.economy.fertilizerCount = 8;
    QVERIFY(!SaveManager::save(path, data));
    QCOMPARE(readFile(path), original);
}

QTEST_GUILESS_MAIN(SaveTests)
#include "save_tests.moc"
