#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include "economy/economyservice.h"
#include "farm/worldstate.h"

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
    static constexpr int SchemaVersion = 4;

    static QString defaultSavePath();
    static QString previousSavePath();
    static QString legacySavePath();
    // Select the newest existing generation, including a lone backup.
    // The caller then tries the primary and its backup within that generation.
    static QString preferredSavePath();
    static bool save(const QString &path, const GameSaveData &data,
                     QString *errorMessage = nullptr,
                     const std::function<bool(const GameSaveData &)> &compatible = {});
    static bool load(const QString &path, GameSaveData *data,
                     QString *errorMessage = nullptr);
};

#endif // SAVEMANAGER_H
