#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include "economyservice.h"
#include "worldstate.h"

#include <QPoint>
#include <QString>
#include <functional>

struct GameSaveData {
    qint64 gameMinutes = 0;
    int lastProcessedDay = 0;
    QPoint playerPosition;
    EconomyState economy;
    WorldStateSnapshot world;
};

class SaveManager {
public:
    static constexpr int SchemaVersion = 3;

    static QString defaultSavePath();
    static QString legacySavePath();
    static bool save(const QString &path, const GameSaveData &data,
                     QString *errorMessage = nullptr,
                     const std::function<bool(const GameSaveData &)> &compatible = {});
    static bool load(const QString &path, GameSaveData *data,
                     QString *errorMessage = nullptr);
};

#endif // SAVEMANAGER_H
