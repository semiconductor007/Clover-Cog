#include "economyservice.h"

#include "farm/cropdefinition.h"
#include "core/gameconstants.h"

#include <QScopedValueRollback>
#include <limits>

namespace {
bool validCropCounts(const QHash<int, int> &counts) {
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        if (CropCatalog::definition(static_cast<CropType>(it.key())).type == CropType::None ||
            it.value() < 0)
            return false;
    }
    return true;
}
}

EconomyService::EconomyService(int inventoryCapacity)
    : m_inventory(inventoryCapacity) {
    m_seedCounts.insert(static_cast<int>(CropType::Wheat),
                        GameConstants::InitialWheatSeeds);
    m_seedCounts.insert(static_cast<int>(CropType::Potato),
                        GameConstants::InitialPotatoSeeds);
}

int EconomyService::seedCount(CropType type) const {
    return m_seedCounts.value(static_cast<int>(type), 0);
}

bool EconomyService::collectHarvest(CropType type, int quantity) {
    return !m_applyingItem && m_inventory.add(type, quantity);
}

bool EconomyService::plantWithSeed(CropType type,
                                   const std::function<bool()> &plantAction) {
    if (m_applyingItem || seedCount(type) <= 0 || !plantAction) {
        return false;
    }
    QScopedValueRollback<bool> applying(m_applyingItem, true);
    if (!plantAction()) {
        return false;
    }
    const int key = static_cast<int>(type);
    --m_seedCounts[key];
    return true;
}

bool EconomyService::fertilizeWithItem(const std::function<bool()> &fertilizeAction) {
    if (m_applyingItem || m_fertilizerCount <= 0 || !fertilizeAction) {
        return false;
    }
    QScopedValueRollback<bool> applying(m_applyingItem, true);
    if (!fertilizeAction()) {
        return false;
    }
    --m_fertilizerCount;
    return true;
}

SeedPurchaseResult EconomyService::buySeeds(CropType type, int quantity) {
    return buySupply(type, quantity, CropCatalog::definition(type).seedPrice);
}

SeedPurchaseResult EconomyService::buyFertilizer(int quantity) {
    return buySupply(CropType::None, quantity, FertilizerPrice);
}

SeedPurchaseResult EconomyService::buySupply(CropType type, int quantity, int price) {
    SeedPurchaseResult result;
    result.type = type;
    result.quantity = quantity;
    const int stock = type == CropType::None ? fertilizerCount() : seedCount(type);
    if (m_applyingItem || price <= 0 || quantity <= 0 ||
        quantity > std::numeric_limits<int>::max() - stock) {
        result.failure = SeedPurchaseFailure::InvalidRequest;
        return result;
    }
    const qint64 totalCost = static_cast<qint64>(price) * quantity;
    if (totalCost > m_coins) {
        result.failure = SeedPurchaseFailure::InsufficientCoins;
        return result;
    }

    m_coins -= static_cast<int>(totalCost);
    if (type == CropType::None) {
        m_fertilizerCount += quantity;
    } else {
        m_seedCounts[static_cast<int>(type)] += quantity;
    }
    result.success = true;
    result.coinsSpent = static_cast<int>(totalCost);
    return result;
}

bool EconomyService::canSell(const QPoint &playerTile, const QRect &marketArea, qint64 revenue) const {
    return !m_applyingItem && marketArea.contains(playerTile) && revenue > 0 &&
           revenue <= std::numeric_limits<int>::max() - m_coins;
}

SaleResult EconomyService::sellCrop(CropType crop, int quantity, const QPoint &playerTile,
                                    const QRect &marketArea) {
    const qint64 revenue = qint64(CropCatalog::definition(crop).sellPrice) * quantity;
    if (!canSell(playerTile, marketArea, revenue) || !m_inventory.remove(crop, quantity)) {
        return {};
    }
    m_coins += static_cast<int>(revenue);
    return {true, quantity, static_cast<int>(revenue)};
}

SaleResult EconomyService::sellAll(const QPoint &playerTile,
                                   const QRect &marketArea) {
    const int itemsSold = m_inventory.usedCapacity();
    qint64 revenue = 0;
    for (CropType type : CropCatalog::cropTypes()) {
        revenue += static_cast<qint64>(m_inventory.count(type)) *
                   CropCatalog::definition(type).sellPrice;
    }
    if (!canSell(playerTile, marketArea, revenue)) {
        return {};
    }

    m_inventory = Inventory(m_inventory.capacity());
    m_coins += static_cast<int>(revenue);
    return {true, itemsSold, static_cast<int>(revenue)};
}

EconomyState EconomyService::state() const {
    return {m_coins, m_inventory.capacity(), m_inventory.counts(),
            m_seedCounts, m_fertilizerCount, m_storage};
}

StorageTransferResult EconomyService::transfer(StorageItemKind kind, CropType crop,
                                              StorageDirection direction, int quantity) {
    const bool depositing = direction == StorageDirection::Deposit;
    if (m_applyingItem || quantity <= 0 || (!depositing && direction != StorageDirection::Withdraw))
        return {false, QStringLiteral("无法转移该数量")};

    // Commit both balances together. A rejected transfer must preserve the entire stack.
    auto candidate = state();
    int *carried = nullptr;
    int *stored = nullptr;
    QString name;
    const int id = static_cast<int>(crop);
    if (kind == StorageItemKind::Fertilizer && crop == CropType::None) {
        carried = &candidate.fertilizerCount;
        stored = &candidate.storage.fertilizerCount;
        name = QStringLiteral("基础肥料");
    } else {
        const auto &definition = CropCatalog::definition(crop);
        if (definition.type == CropType::None)
            return {false, QStringLiteral("未知的储物物品")};
        name = definition.localizedName;
        if (kind == StorageItemKind::Harvest) {
            carried = &candidate.inventoryCounts[id];
            stored = &candidate.storage.harvestCounts[id];
        } else if (kind == StorageItemKind::Seed) {
            carried = &candidate.seedCounts[id];
            stored = &candidate.storage.seedCounts[id];
            name += QStringLiteral("种子");
        } else {
            return {false, QStringLiteral("未知的储物类别")};
        }
    }
    int &source = depositing ? *carried : *stored;
    int &destination = depositing ? *stored : *carried;
    if (quantity > source)
        return {false, QStringLiteral("物品不足，只有 %1 份%2").arg(source).arg(name)};
    if (!depositing && kind == StorageItemKind::Harvest && quantity > m_inventory.remainingCapacity())
        return {false, QStringLiteral("背包只剩 %1 格，无法取出 %2 份").arg(m_inventory.remainingCapacity()).arg(quantity)};
    if (quantity > std::numeric_limits<int>::max() - destination)
        return {false, QStringLiteral("物品数量过大，无法转移")};
    source -= quantity;
    destination += quantity;
    if (!restore(candidate))
        return {false, QStringLiteral("无法转移物品，库存已保留")};
    return {true, QStringLiteral("%1%2 × %3")
                      .arg(depositing ? QStringLiteral("已存入") : QStringLiteral("已取出"), name)
                      .arg(quantity)};
}

bool EconomyService::restore(const EconomyState &state) {
    if (m_applyingItem || state.coins < 0 || state.fertilizerCount < 0 ||
        state.storage.fertilizerCount < 0 || !validCropCounts(state.seedCounts) ||
        !validCropCounts(state.storage.harvestCounts) || !validCropCounts(state.storage.seedCounts)) {
        return false;
    }

    Inventory restoredInventory;
    if (!restoredInventory.restore(state.inventoryCapacity,
                                   state.inventoryCounts)) {
        return false;
    }
    m_coins = state.coins;
    m_fertilizerCount = state.fertilizerCount;
    m_inventory = restoredInventory;
    m_seedCounts = state.seedCounts;
    m_storage = state.storage;
    for (CropType type : CropCatalog::cropTypes()) {
        if (!m_seedCounts.contains(static_cast<int>(type))) {
            m_seedCounts.insert(static_cast<int>(type), 0);
        }
    }
    return true;
}
