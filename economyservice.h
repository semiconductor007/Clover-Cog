#ifndef ECONOMYSERVICE_H
#define ECONOMYSERVICE_H

#include "inventory.h"

#include <QPoint>
#include <QRect>
#include <QHash>
#include <functional>

struct SaleResult {
    bool success = false;
    int itemsSold = 0;
    int coinsEarned = 0;
};

enum class SeedPurchaseFailure {
    None,
    InvalidRequest,
    InsufficientCoins
};

struct SeedPurchaseResult {
    bool success = false;
    CropType type = CropType::None;
    int quantity = 0;
    int coinsSpent = 0;
    SeedPurchaseFailure failure = SeedPurchaseFailure::None;
};

struct EconomyState {
    int coins = 0;
    int inventoryCapacity = 0;
    QHash<int, int> inventoryCounts;
    QHash<int, int> seedCounts;
};

class EconomyService {
public:
    explicit EconomyService(int inventoryCapacity = 12);

    const Inventory &inventory() const { return m_inventory; }
    int coins() const { return m_coins; }
    int seedCount(CropType type) const;

    bool collectHarvest(CropType type, int quantity);
    bool plantWithSeed(CropType type, const std::function<bool()> &plantAction);
    SeedPurchaseResult buySeeds(CropType type, int quantity);
    SaleResult sellAll(const QPoint &playerTile, const QRect &marketArea);
    EconomyState state() const;
    bool restore(const EconomyState &state);

private:
    Inventory m_inventory;
    int m_coins = 0;
    QHash<int, int> m_seedCounts;
};

#endif // ECONOMYSERVICE_H
