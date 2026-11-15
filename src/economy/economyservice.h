#ifndef ECONOMYSERVICE_H
#define ECONOMYSERVICE_H

#include "inventory.h"

#include <QPoint>
#include <QRect>
#include <QHash>
#include <functional>
#include <QMetaType>
#include "player/tooltype.h"

struct InventoryStack {
    int item = 0;
    int quantity = 0;
    bool operator==(const InventoryStack &other) const { return item == other.item && quantity == other.quantity; }
};

namespace StackItem {
inline constexpr int Hoe = 1001, Water = 1002, Fertilizer = 4001;
inline int seed(CropType crop) { return 2000 + static_cast<int>(crop); }
inline int harvest(CropType crop) { return 3000 + static_cast<int>(crop); }
inline CropType crop(int item) { return static_cast<CropType>(item % 1000); }
inline bool isTool(int item) { return item == Hoe || item == Water; }
inline bool valid(int item) {
    return item == 0 || isTool(item) || item == Fertilizer ||
        ((item/1000 == 2 || item/1000 == 3) && CropCatalog::definition(crop(item)).type != CropType::None);
}
inline ToolType tool(int item) {
    if (item == Hoe) return ToolType::Hoe;
    if (item == Water) return ToolType::WateringCan;
    if (item == Fertilizer) return ToolType::Fertilizer;
    return item/1000 == 2 ? toolForSeed(crop(item)) : ToolType::EmptyHand;
}
inline int fromTool(ToolType type) {
    if (type == ToolType::Hoe) return Hoe;
    if (type == ToolType::WateringCan) return Water;
    if (type == ToolType::Fertilizer) return Fertilizer;
    return seedCropForTool(type) != CropType::None ? seed(seedCropForTool(type)) : 0;
}
}

struct StackMove {
    bool sourceChest = false, targetChest = false;
    int source = -1, target = -1, quantity = 0;
    quint64 revision = 0;
    InventoryStack expected;
};
Q_DECLARE_METATYPE(StackMove)

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
    QVector<InventoryStack> bagSlots;
    QVector<InventoryStack> chestSlots;
};

class EconomyService {
public:
    static constexpr int BagSlots = 21;
    const QVector<InventoryStack> &bagSlots() const { return m_bagSlots; }
    const QVector<InventoryStack> &chestSlots() const { return m_chestSlots; }
    quint64 revision() const { return m_revision; }
    bool moveStack(const StackMove &move);
    void setEquippedSlot(int slot) { m_equippedSlot = slot >= 0 && slot < 7 ? slot : -1; }
    static bool layoutMatches(const EconomyState &state);
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
    static bool reconcile(EconomyState &state);
    void changed();
    void consumeSlot(int item);
    QVector<InventoryStack> m_bagSlots, m_chestSlots;
    quint64 m_revision = 0;
    int m_equippedSlot = -1;
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
