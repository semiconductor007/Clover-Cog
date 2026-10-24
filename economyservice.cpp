#include "economyservice.h"

#include "cropdefinition.h"
#include "gameconstants.h"

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
    return m_inventory.add(type, quantity);
}

bool EconomyService::plantWithSeed(CropType type,
                                   const std::function<bool()> &plantAction) {
    if (type == CropType::None || seedCount(type) <= 0 || !plantAction) {
        return false;
    }
    if (!plantAction()) {
        return false;
    }
    const int key = static_cast<int>(type);
    --m_seedCounts[key];
    return true;
}

SeedPurchaseResult EconomyService::buySeeds(CropType type, int quantity) {
    SeedPurchaseResult result;
    result.type = type;
    result.quantity = quantity;
    const CropDefinition &definition = CropCatalog::definition(type);
    if (type == CropType::None || quantity <= 0 || definition.seedPrice <= 0) {
        result.failure = SeedPurchaseFailure::InvalidRequest;
        return result;
    }

    const qint64 totalCost = static_cast<qint64>(definition.seedPrice) * quantity;
    if (totalCost > m_coins) {
        result.failure = SeedPurchaseFailure::InsufficientCoins;
        return result;
    }

    m_coins -= static_cast<int>(totalCost);
    m_seedCounts[static_cast<int>(type)] += quantity;
    result.success = true;
    result.coinsSpent = static_cast<int>(totalCost);
    return result;
}

SaleResult EconomyService::sellAll(const QPoint &playerTile,
                                   const QRect &marketArea) {
    SaleResult result;
    if (!marketArea.contains(playerTile)) {
        return result;
    }

    for (CropType type : CropCatalog::cropTypes()) {
        const int quantity = m_inventory.count(type);
        if (quantity <= 0) {
            continue;
        }
        const int value = quantity * CropCatalog::definition(type).sellPrice;
        if (m_inventory.remove(type, quantity)) {
            result.itemsSold += quantity;
            result.coinsEarned += value;
        }
    }

    result.success = result.itemsSold > 0;
    m_coins += result.coinsEarned;
    return result;
}

EconomyState EconomyService::state() const {
    return {m_coins, m_inventory.capacity(), m_inventory.counts(), m_seedCounts};
}

bool EconomyService::restore(const EconomyState &state) {
    if (state.coins < 0) {
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
    m_inventory = restoredInventory;
    m_seedCounts = state.seedCounts;
    for (CropType type : CropCatalog::cropTypes()) {
        if (!m_seedCounts.contains(static_cast<int>(type))) {
            m_seedCounts.insert(static_cast<int>(type), 0);
        }
    }
    return true;
}
