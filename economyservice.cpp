#include "economyservice.h"

#include "cropdefinition.h"
#include "gameconstants.h"

#include <QScopedValueRollback>
#include <limits>

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

SaleResult EconomyService::sellAll(const QPoint &playerTile,
                                   const QRect &marketArea) {
    if (m_applyingItem || !marketArea.contains(playerTile)) {
        return {};
    }

    const int itemsSold = m_inventory.usedCapacity();
    qint64 revenue = 0;
    for (CropType type : CropCatalog::cropTypes()) {
        revenue += static_cast<qint64>(m_inventory.count(type)) *
                   CropCatalog::definition(type).sellPrice;
    }
    if (itemsSold == 0 || revenue > std::numeric_limits<int>::max() - m_coins) {
        return {};
    }

    m_inventory = Inventory(m_inventory.capacity());
    m_coins += static_cast<int>(revenue);
    return {true, itemsSold, static_cast<int>(revenue)};
}

EconomyState EconomyService::state() const {
    return {m_coins, m_inventory.capacity(), m_inventory.counts(),
            m_seedCounts, m_fertilizerCount};
}

bool EconomyService::restore(const EconomyState &state) {
    if (m_applyingItem || state.coins < 0 || state.fertilizerCount < 0) {
        return false;
    }
    for (auto it = state.seedCounts.cbegin(); it != state.seedCounts.cend(); ++it) {
        const CropType type = static_cast<CropType>(it.key());
        if (CropCatalog::definition(type).type == CropType::None || it.value() < 0) {
            return false;
        }
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
    for (CropType type : CropCatalog::cropTypes()) {
        if (!m_seedCounts.contains(static_cast<int>(type))) {
            m_seedCounts.insert(static_cast<int>(type), 0);
        }
    }
    return true;
}
