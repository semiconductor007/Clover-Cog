#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include "economyservice.h"
#include "worldstate.h"

#include <QPoint>
#include <QString>

struct GameSaveData {
    qint64 gameMinutes = 0;
    int lastProcessedDay = 0;
    QPoint playerPosition;
    EconomyState economy;
    WorldStateSnapshot world;
};

class SaveManager {
public:
    static constexpr int SchemaVersion = 1;

    static QString defaultSavePath();
    static bool save(const QString &path, const GameSaveData &data,
                     QString *errorMessage = nullptr);
    static bool load(const QString &path, GameSaveData *data,
                     QString *errorMessage = nullptr);
};

#endif // SAVEMANAGER_H
