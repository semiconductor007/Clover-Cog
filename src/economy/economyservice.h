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

enum class StorageItemKind { Harvest, Seed, Fertilizer };
enum class StorageDirection { Deposit, Withdraw };

struct StorageStock {
    QHash<int, int> harvestCounts;
    QHash<int, int> seedCounts;
    int fertilizerCount = 0;
};

struct StorageTransferResult {
    bool success = false;
    QString message;
};

struct EconomyState {
    int coins = 0;
    int inventoryCapacity = 0;
    QHash<int, int> inventoryCounts;
    QHash<int, int> seedCounts;
    int fertilizerCount = 0;
    StorageStock storage;
};

class EconomyService {
public:
    static constexpr int FertilizerPrice = 1;

    explicit EconomyService(int inventoryCapacity = 12);

    const Inventory &inventory() const { return m_inventory; }
    int coins() const { return m_coins; }
    int seedCount(CropType type) const;
    int fertilizerCount() const { return m_fertilizerCount; }
    const StorageStock &storage() const { return m_storage; }
    StorageTransferResult transfer(StorageItemKind kind, CropType crop,
                                   StorageDirection direction, int quantity);

    bool collectHarvest(CropType type, int quantity);
    bool plantWithSeed(CropType type, const std::function<bool()> &plantAction);
    bool fertilizeWithItem(const std::function<bool()> &fertilizeAction);
    SeedPurchaseResult buyFertilizer(int quantity);
    SeedPurchaseResult buySeeds(CropType type, int quantity);
    SaleResult sellCrop(CropType crop, int quantity, const QPoint &playerTile, const QRect &marketArea);
    SaleResult sellAll(const QPoint &playerTile, const QRect &marketArea);
    EconomyState state() const;
    bool restore(const EconomyState &state);

private:
    bool canSell(const QPoint &playerTile, const QRect &marketArea, qint64 revenue) const;
    SeedPurchaseResult buySupply(CropType type, int quantity, int price);

    Inventory m_inventory;
    int m_coins = 0;
    QHash<int, int> m_seedCounts;
    int m_fertilizerCount = 0;
    StorageStock m_storage;
    bool m_applyingItem = false;
};

#endif // ECONOMYSERVICE_H
