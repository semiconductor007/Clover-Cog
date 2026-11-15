#include "savemanager.h"

#include "farm/cropdefinition.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <limits>

namespace {

QString cropId(CropType type) {
    return CropCatalog::definition(type).id;
}

CropType cropType(const QString &id) {
    return CropCatalog::fromId(id);
}

QJsonObject pointToJson(const QPoint &point) {
    return {{QStringLiteral("x"), point.x()},
            {QStringLiteral("y"), point.y()}};
}

bool pointFromJson(const QJsonValue &value, QPoint *point) {
    if (!point || !value.isObject()) {
        return false;
    }
    const QJsonObject object = value.toObject();
    const int x = object.value(QStringLiteral("x")).toInt(-1);
    const int y = object.value(QStringLiteral("y")).toInt(-1);
    if (x < 0 || y < 0) {
        return false;
    }
    *point = QPoint(x, y);
    return true;
}

QJsonObject countsToJson(const QHash<int, int> &counts) {
    QJsonObject result;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        const QString id = cropId(static_cast<CropType>(it.key()));
        if (!id.isEmpty()) {
            result.insert(id, it.value());
        }
    }
    return result;
}

bool countsFromJson(const QJsonValue &value, QHash<int, int> *counts) {
    if (!counts || !value.isObject()) {
        return false;
    }
    QHash<int, int> parsed;
    const QJsonObject object = value.toObject();
    for (auto it = object.begin(); it != object.end(); ++it) {
        const CropType type = cropType(it.key());
        if (type == CropType::None || !it.value().isDouble() ||
            it.value().toInt(-1) < 0) {
            return false;
        }
        parsed.insert(static_cast<int>(type), it.value().toInt());
    }
    *counts = parsed;
    return true;
}

QJsonArray layerToJson(const QVector<QVector<int>> &layer) {
    QJsonArray rows;
    for (const QVector<int> &row : layer) {
        QJsonArray values;
        for (int gid : row) {
            values.append(gid);
        }
        rows.append(values);
    }
    return rows;
}

bool layerFromJson(const QJsonValue &value, QVector<QVector<int>> *layer) {
    if (!layer || !value.isArray()) {
        return false;
    }
    QVector<QVector<int>> parsed;
    for (const QJsonValue &rowValue : value.toArray()) {
        if (!rowValue.isArray()) {
            return false;
        }
        QVector<int> row;
        for (const QJsonValue &gidValue : rowValue.toArray()) {
            if (!gidValue.isDouble() || gidValue.toInt(-1) < 0) {
                return false;
            }
            row.append(gidValue.toInt());
        }
        parsed.append(row);
    }
    *layer = parsed;
    return true;
}

void setError(QString *errorMessage, const QString &message) {
    if (errorMessage) {
        *errorMessage = message;
    }
}

// Check gameplay invariants before accepting a file or rotating it into the backup.
bool validSnapshot(const GameSaveData &data, QString *errorMessage) {
    const qint64 day = data.gameMinutes / (24 * 60);
    if (data.gameMinutes < 0 || day > std::numeric_limits<int>::max() ||
        data.lastProcessedDay < 0 || data.lastProcessedDay > day ||
        data.world.lastCompletedDay != data.lastProcessedDay - 1 ||
        data.playerPosition.x() < 0 || data.playerPosition.y() < 0) {
        setError(errorMessage, QStringLiteral("存档日期或角色位置无效"));
        return false;
    }

    EconomyService economy;
    if (!economy.restore(data.economy)) {
        setError(errorMessage, QStringLiteral("存档中的金币或库存无效"));
        return false;
    }

    const auto &layer = data.world.cultivatedLayer;
    if (layer.isEmpty() || layer.first().isEmpty()) {
        setError(errorMessage, QStringLiteral("存档缺少地图"));
        return false;
    }
    for (const auto &row : layer) {
        if (row.size() != layer.first().size()) {
            setError(errorMessage, QStringLiteral("存档中的地图尺寸无效"));
            return false;
        }
    }
    for (const auto &crop : data.world.crops) {
        if (crop.plantedDay > day) {
            setError(errorMessage, QStringLiteral("作物播种时间晚于存档日期"));
            return false;
        }
    }

    MapData map;
    map.mapWidth = layer.first().size();
    map.mapHeight = layer.size();
    WorldState world;
    world.initialize(map);
    if (!world.restore(data.world)) {
        setError(errorMessage, QStringLiteral("存档中的农田状态无效"));
        return false;
    }
    return true;
}

QByteArray encodeSave(const GameSaveData &data) {
    EconomyService canonical;
    canonical.restore(data.economy);
    const auto layout=canonical.state();
    const auto encodeSlots=[](const QVector<InventoryStack> &cells) {
        QJsonArray array;
        for (const auto &cell : cells) array.append(QJsonObject{{"item",cell.item},{"quantity",cell.quantity}});
        return array;
    };
    QJsonObject economy;
    economy.insert(QStringLiteral("slotLayoutVersion"),1);
    economy.insert(QStringLiteral("bagSlots"),encodeSlots(layout.bagSlots));
    economy.insert(QStringLiteral("chestSlots"),encodeSlots(layout.chestSlots));
    economy.insert(QStringLiteral("coins"), data.economy.coins);
    economy.insert(QStringLiteral("inventoryCapacity"),
                   data.economy.inventoryCapacity);
    economy.insert(QStringLiteral("inventory"),
                   countsToJson(data.economy.inventoryCounts));
    economy.insert(QStringLiteral("seeds"),
                   countsToJson(data.economy.seedCounts));

    economy.insert(QStringLiteral("fertilizerCount"), data.economy.fertilizerCount);
    const StorageStock &storage = data.economy.storage;
    economy.insert(QStringLiteral("storage"), QJsonObject{
        {QStringLiteral("harvest"), countsToJson(storage.harvestCounts)},
        {QStringLiteral("seeds"), countsToJson(storage.seedCounts)},
        {QStringLiteral("fertilizerCount"), storage.fertilizerCount}
    });

    QJsonArray crops;
    for (const SavedCropState &crop : data.world.crops) {
        QJsonObject object = pointToJson(crop.tilePos);
        object.insert(QStringLiteral("type"), cropId(crop.type));
        object.insert(QStringLiteral("plantedDay"), crop.plantedDay);
        object.insert(QStringLiteral("growthDays"), crop.growthDays);
        object.insert(QStringLiteral("fertilizerApplied"), crop.fertilizerApplied);
        object.insert(QStringLiteral("fertilizerBonusPending"), crop.fertilizerBonusPending);
        crops.append(object);
    }
    QJsonArray wateredTiles;
    for (const QPoint &point : data.world.wateredTiles) {
        wateredTiles.append(pointToJson(point));
    }
    QJsonObject world;
    world.insert(QStringLiteral("cultivatedLayer"),
                 layerToJson(data.world.cultivatedLayer));
    world.insert(QStringLiteral("crops"), crops);
    world.insert(QStringLiteral("wateredTiles"), wateredTiles);
    world.insert(QStringLiteral("lastCompletedDay"),
                 data.world.lastCompletedDay);

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), SaveManager::SchemaVersion);
    root.insert(QStringLiteral("gameMinutes"),
                QString::number(data.gameMinutes));
    root.insert(QStringLiteral("lastProcessedDay"), data.lastProcessedDay);
    root.insert(QStringLiteral("playerPosition"),
                pointToJson(data.playerPosition));
    root.insert(QStringLiteral("economy"), economy);
    root.insert(QStringLiteral("world"), world);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool decodeSave(const QByteArray &contents, GameSaveData *data, QString *errorMessage) {
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(contents, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    const int version = root.value(QStringLiteral("schemaVersion")).toInt(-1);
    if (version != 2 && version != 3 && version != SaveManager::SchemaVersion) {
        setError(errorMessage, QStringLiteral("不支持的存档版本"));
        return false;
    }

    bool minutesOk = false;
    GameSaveData parsed;
    parsed.gameMinutes =
        root.value(QStringLiteral("gameMinutes")).toString().toLongLong(&minutesOk);
    parsed.lastProcessedDay =
        root.value(QStringLiteral("lastProcessedDay")).toInt(-1);
    if (!minutesOk || parsed.gameMinutes < 0 || parsed.lastProcessedDay < 0 ||
        !pointFromJson(root.value(QStringLiteral("playerPosition")),
                       &parsed.playerPosition)) {
        setError(errorMessage, QStringLiteral("时间或角色位置无效"));
        return false;
    }

    const QJsonObject economy = root.value(QStringLiteral("economy")).toObject();
    parsed.economy.coins = economy.value(QStringLiteral("coins")).toInt(-1);
    parsed.economy.inventoryCapacity =
        economy.value(QStringLiteral("inventoryCapacity")).toInt(-1);
    const QJsonValue fertilizer = economy.value(QStringLiteral("fertilizerCount"));
    parsed.economy.fertilizerCount = (version == 2 && fertilizer.isUndefined())
        ? 0 : fertilizer.toInt(-1);
    if (parsed.economy.coins < 0 || parsed.economy.inventoryCapacity < 0 ||
        parsed.economy.fertilizerCount < 0 ||
        !countsFromJson(economy.value(QStringLiteral("inventory")),
                        &parsed.economy.inventoryCounts) ||
        !countsFromJson(economy.value(QStringLiteral("seeds")),
                        &parsed.economy.seedCounts)) {
        setError(errorMessage, QStringLiteral("存档中的库存或金币无效"));
        return false;
    }

    if (version >= 4) {
        const QJsonObject storage = economy.value(QStringLiteral("storage")).toObject();
        auto &stock = parsed.economy.storage;
        stock.fertilizerCount = storage.value(QStringLiteral("fertilizerCount")).toInt(-1);
        if (storage.size() != 3 || stock.fertilizerCount < 0 ||
            !countsFromJson(storage.value(QStringLiteral("harvest")), &stock.harvestCounts) ||
            !countsFromJson(storage.value(QStringLiteral("seeds")), &stock.seedCounts)) {
            setError(errorMessage, QStringLiteral("储物箱库存无效"));
            return false;
        }
    }

    if (version>=4 && (economy.contains(QStringLiteral("slotLayoutVersion")) || economy.contains(QStringLiteral("bagSlots")) ||
        economy.contains(QStringLiteral("chestSlots")))) {
        const auto decodeSlots=[](const QJsonValue &value, QVector<InventoryStack> &cells) {
            if (!value.isArray() || value.toArray().size()>65536) return false;
            for (const auto &entry : value.toArray()) {
                if (!entry.isObject()) return false;
                const auto object=entry.toObject();
                const auto item=object.value("item"), quantity=object.value("quantity");
                if (object.size()!=2 || !item.isDouble() || !quantity.isDouble() ||
                    item.toInt(-1)<0 || quantity.toInt(-1)<0 || item.toDouble()!=item.toInt(-1) ||
                    quantity.toDouble()!=quantity.toInt(-1)) return false;
                cells.append({item.toInt(),quantity.toInt()});
            }
            return true;
        };
        if (economy.value("slotLayoutVersion").toInt(-1)!=1 ||
            !decodeSlots(economy.value("bagSlots"),parsed.economy.bagSlots) ||
            !decodeSlots(economy.value("chestSlots"),parsed.economy.chestSlots) ||
            !EconomyService::layoutMatches(parsed.economy)) {
            setError(errorMessage,QStringLiteral("背包格位与实际库存不一致")); return false;
        }
    }

    const QJsonObject world = root.value(QStringLiteral("world")).toObject();
    parsed.world.lastCompletedDay =
        world.value(QStringLiteral("lastCompletedDay")).toInt(-2);
    if (!layerFromJson(world.value(QStringLiteral("cultivatedLayer")),
                       &parsed.world.cultivatedLayer) ||
        parsed.world.lastCompletedDay < -1 ||
        !world.value(QStringLiteral("crops")).isArray() ||
        !world.value(QStringLiteral("wateredTiles")).isArray()) {
        setError(errorMessage, QStringLiteral("地图存档无效"));
        return false;
    }
    for (const QJsonValue &cropValue :
         world.value(QStringLiteral("crops")).toArray()) {
        if (!cropValue.isObject()) {
            setError(errorMessage, QStringLiteral("作物存档无效"));
            return false;
        }
        const QJsonObject object = cropValue.toObject();
        SavedCropState crop;
        if (!pointFromJson(object, &crop.tilePos)) {
            setError(errorMessage, QStringLiteral("作物位置无效"));
            return false;
        }
        crop.type = cropType(object.value(QStringLiteral("type")).toString());
        crop.plantedDay = object.value(QStringLiteral("plantedDay")).toInt(-1);
        crop.growthDays = object.value(QStringLiteral("growthDays")).toInt(-1);
        const QJsonValue applied = object.value(QStringLiteral("fertilizerApplied"));
        const QJsonValue pending = object.value(QStringLiteral("fertilizerBonusPending"));
        if (crop.type == CropType::None || crop.plantedDay < 0 ||
            crop.growthDays < 0 || !applied.isBool() || !pending.isBool() ||
            (pending.toBool() && !applied.toBool())) {
            setError(errorMessage, QStringLiteral("作物状态无效"));
            return false;
        }
        crop.fertilizerApplied = applied.toBool();
        crop.fertilizerBonusPending = pending.toBool();
        parsed.world.crops.append(crop);
    }
    for (const QJsonValue &pointValue :
         world.value(QStringLiteral("wateredTiles")).toArray()) {
        QPoint point;
        if (!pointFromJson(pointValue, &point)) {
            setError(errorMessage, QStringLiteral("浇水位置无效"));
            return false;
        }
        parsed.world.wateredTiles.insert(point);
    }

    if (!validSnapshot(parsed, errorMessage)) {
        return false;
    }
    *data = parsed;
    return true;
}

bool readSave(const QString &path, GameSaveData *data, QByteArray *original,
              QString *errorMessage) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, file.errorString());
        return false;
    }
    const QByteArray contents = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setError(errorMessage, file.errorString());
        return false;
    }
    if (!decodeSave(contents, data, errorMessage)) {
        return false;
    }
    if (original) {
        *original = contents;
    }
    return true;
}

bool writeAtomically(const QString &path, const QByteArray &contents, QString *errorMessage) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() ||
        !file.commit()) {
        setError(errorMessage, file.errorString());
        return false;
    }
    return true;
}

} // namespace

QString SaveManager::defaultSavePath() {
    const QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    return dir.filePath(QStringLiteral("farm-v4.json"));
}

QString SaveManager::previousSavePath() {
    const QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    return dir.filePath(QStringLiteral("farm-v3.json"));
}

QString SaveManager::legacySavePath() {
    const QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    return dir.filePath(QStringLiteral("savegame.json"));
}

QString SaveManager::preferredSavePath() {
    for (const QString &path : {defaultSavePath(), previousSavePath(), legacySavePath()}) {
        if (QFileInfo::exists(path) || QFileInfo::exists(path + QStringLiteral(".bak")))
            return path;
    }
    return {};
}

bool SaveManager::save(const QString &path, const GameSaveData &data,
                       QString *errorMessage,
                       const std::function<bool(const GameSaveData &)> &compatible) {
    if (!validSnapshot(data, errorMessage)) {
        return false;
    }
    if (compatible && !compatible(data)) {
        setError(errorMessage, QStringLiteral("当前农场状态与地图或背包容量不符"));
        return false;
    }
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        setError(errorMessage, QStringLiteral("无法建立存档目录"));
        return false;
    }

    const QByteArray contents = encodeSave(data);
    const QString backupPath = path + QStringLiteral(".bak");
    const bool hasPrimary = QFileInfo::exists(path);
    const bool hasBackup = QFileInfo::exists(backupPath);
    const auto &layer = data.world.cultivatedLayer;
    const auto recoverable = [&](const QString &candidate, QByteArray *original) {
        GameSaveData previous;
        return readSave(candidate, &previous, original, nullptr) &&
               previous.world.cultivatedLayer.size() == layer.size() &&
               previous.world.cultivatedLayer.first().size() == layer.first().size() &&
               (!compatible || compatible(previous));
    };

    QByteArray previous;
    if (hasPrimary && recoverable(path, &previous)) {
        if (!writeAtomically(backupPath, previous, errorMessage)) {
            return false;
        }
    } else if (hasPrimary || hasBackup) {
        if (!hasBackup || !recoverable(backupPath, nullptr)) {
            setError(errorMessage, QStringLiteral("未找到有效备份，已保留已有存档"));
            return false;
        }
    } else if (!writeAtomically(backupPath, contents, errorMessage)) {
        return false;
    }
    return writeAtomically(path, contents, errorMessage);
}

bool SaveManager::load(const QString &path, GameSaveData *data, QString *errorMessage) {
    if (!data) {
        setError(errorMessage, QStringLiteral("缺少存档读取目标"));
        return false;
    }
    return readSave(path, data, nullptr, errorMessage);
}
