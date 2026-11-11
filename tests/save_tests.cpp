#include "savemanager.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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
    void invalidV3StockRejected_data();
    void invalidV3StockRejected();
    void backupSurvivesUnreadablePrimary();
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
    QCOMPARE(current.fileName(), QStringLiteral("farm-v3.json"));
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
    v2.insert("economy", economy);
    const QByteArray previous = QJsonDocument(v2).toJson();
    QVERIFY(writeFile(legacyPath, previous));
    GameSaveData loaded;
    QVERIFY(SaveManager::load(legacyPath, &loaded));
    QCOMPARE(loaded.economy.fertilizerCount, 0);
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
    auto economy = root.value("economy").toObject();
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

void SaveTests::backupSurvivesRuntimeRejectedPrimary() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("farm.json");
    const QString backup = path + ".bak";
    auto data = newFarm();
    QVERIFY(SaveManager::save(path, data));
    const QByteArray original = readFile(path);
    auto root = QJsonDocument::fromJson(original).object();
    root.insert("playerPosition", QJsonObject{{"x", 1000}, {"y", 0}});
    QVERIFY(writeFile(path, QJsonDocument(root).toJson()));
    QVERIFY(SaveManager::load(path, &data));
    QVERIFY(data.playerPosition.x() > 2 * 16);
    QVERIFY(SaveManager::load(backup, &data));
    data.economy.coins = 6;
    QVERIFY(SaveManager::save(path, data, nullptr, true));
    QCOMPARE(readFile(backup), original);
    QVERIFY(SaveManager::load(path, &data));
    QCOMPARE(data.economy.coins, 6);
    QCOMPARE(data.playerPosition, QPoint(0, 0));
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

QTEST_APPLESS_MAIN(SaveTests)
#include "save_tests.moc"
