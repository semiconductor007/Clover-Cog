#include "savemanager.h"

#include "cropdefinition.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

QString cropId(CropType type) {
    return CropCatalog::definition(type).id;
}

CropType cropType(const QString &id) {
    for (CropType type : CropCatalog::cropTypes()) {
        if (CropCatalog::definition(type).id == id) {
            return type;
        }
    }
    return CropType::None;
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
    if (!object.value(QStringLiteral("x")).isDouble() ||
        !object.value(QStringLiteral("y")).isDouble()) {
        return false;
    }
    *point = QPoint(object.value(QStringLiteral("x")).toInt(),
                    object.value(QStringLiteral("y")).toInt());
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

} // namespace

QString SaveManager::defaultSavePath() {
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(directory).filePath(QStringLiteral("savegame.json"));
}

bool SaveManager::save(const QString &path, const GameSaveData &data,
                       QString *errorMessage) {
    QJsonObject economy;
    economy.insert(QStringLiteral("coins"), data.economy.coins);
    economy.insert(QStringLiteral("inventoryCapacity"),
                   data.economy.inventoryCapacity);
    economy.insert(QStringLiteral("inventory"),
                   countsToJson(data.economy.inventoryCounts));
    economy.insert(QStringLiteral("seeds"),
                   countsToJson(data.economy.seedCounts));

    QJsonArray crops;
    for (const SavedCropState &crop : data.world.crops) {
        QJsonObject object = pointToJson(crop.tilePos);
        object.insert(QStringLiteral("type"), cropId(crop.type));
        object.insert(QStringLiteral("plantedDay"), crop.plantedDay);
        object.insert(QStringLiteral("growthDays"), crop.growthDays);
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
    root.insert(QStringLiteral("schemaVersion"), SchemaVersion);
    root.insert(QStringLiteral("gameMinutes"),
                QString::number(data.gameMinutes));
    root.insert(QStringLiteral("lastProcessedDay"), data.lastProcessedDay);
    root.insert(QStringLiteral("playerPosition"),
                pointToJson(data.playerPosition));
    root.insert(QStringLiteral("economy"), economy);
    root.insert(QStringLiteral("world"), world);

    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        setError(errorMessage, QStringLiteral("Cannot create save directory"));
        return false;
    }
    if (QFileInfo::exists(path)) {
        QFile existing(path);
        if (existing.open(QIODevice::ReadOnly)) {
            const QJsonDocument existingDocument =
                QJsonDocument::fromJson(existing.readAll());
            if (existingDocument.isObject() &&
                existingDocument.object()
                        .value(QStringLiteral("schemaVersion"))
                        .toInt(-1) == SchemaVersion) {
                const QString backupPath = path + QStringLiteral(".bak");
                QFile::remove(backupPath);
                QFile::copy(path, backupPath);
            }
        }
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(errorMessage, file.errorString());
        return false;
    }
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size() || !file.commit()) {
        setError(errorMessage, file.errorString());
        return false;
    }
    return true;
}

bool SaveManager::load(const QString &path, GameSaveData *data,
                       QString *errorMessage) {
    if (!data) {
        setError(errorMessage, QStringLiteral("Missing save destination"));
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, file.errorString());
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt(-1) !=
        SchemaVersion) {
        setError(errorMessage, QStringLiteral("Unsupported save version"));
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
        setError(errorMessage, QStringLiteral("Invalid time or player state"));
        return false;
    }

    const QJsonObject economy = root.value(QStringLiteral("economy")).toObject();
    parsed.economy.coins = economy.value(QStringLiteral("coins")).toInt(-1);
    parsed.economy.inventoryCapacity =
        economy.value(QStringLiteral("inventoryCapacity")).toInt(-1);
    if (parsed.economy.coins < 0 || parsed.economy.inventoryCapacity < 0 ||
        !countsFromJson(economy.value(QStringLiteral("inventory")),
                        &parsed.economy.inventoryCounts) ||
        !countsFromJson(economy.value(QStringLiteral("seeds")),
                        &parsed.economy.seedCounts)) {
        setError(errorMessage, QStringLiteral("Invalid economy state"));
        return false;
    }

    const QJsonObject world = root.value(QStringLiteral("world")).toObject();
    parsed.world.lastCompletedDay =
        world.value(QStringLiteral("lastCompletedDay")).toInt(-2);
    if (!layerFromJson(world.value(QStringLiteral("cultivatedLayer")),
                       &parsed.world.cultivatedLayer) ||
        parsed.world.lastCompletedDay < -1 ||
        !world.value(QStringLiteral("crops")).isArray() ||
        !world.value(QStringLiteral("wateredTiles")).isArray()) {
        setError(errorMessage, QStringLiteral("Invalid world state"));
        return false;
    }
    for (const QJsonValue &cropValue :
         world.value(QStringLiteral("crops")).toArray()) {
        if (!cropValue.isObject()) {
            setError(errorMessage, QStringLiteral("Invalid crop state"));
            return false;
        }
        const QJsonObject object = cropValue.toObject();
        SavedCropState crop;
        if (!pointFromJson(object, &crop.tilePos)) {
            setError(errorMessage, QStringLiteral("Invalid crop position"));
            return false;
        }
        crop.type = cropType(object.value(QStringLiteral("type")).toString());
        crop.plantedDay = object.value(QStringLiteral("plantedDay")).toInt(-1);
        crop.growthDays = object.value(QStringLiteral("growthDays")).toInt(-1);
        if (crop.type == CropType::None || crop.plantedDay < 0 ||
            crop.growthDays < 0) {
            setError(errorMessage, QStringLiteral("Invalid crop values"));
            return false;
        }
        parsed.world.crops.append(crop);
    }
    for (const QJsonValue &pointValue :
         world.value(QStringLiteral("wateredTiles")).toArray()) {
        QPoint point;
        if (!pointFromJson(pointValue, &point)) {
            setError(errorMessage, QStringLiteral("Invalid watered tile"));
            return false;
        }
        parsed.world.wateredTiles.insert(point);
    }

    *data = parsed;
    return true;
}
