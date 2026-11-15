#include "economyservice.h"

#include "farm/cropdefinition.h"
#include "core/gameconstants.h"

#include <QScopedValueRollback>
#include <limits>
#include <algorithm>

namespace {
QHash<int,qint64> totals(const EconomyState &state, bool chest) {
    QHash<int,qint64> result;
    for (CropType crop : CropCatalog::cropTypes()) {
        const int id=static_cast<int>(crop);
        result[StackItem::seed(crop)] = chest ? state.storage.seedCounts.value(id) : state.seedCounts.value(id);
        result[StackItem::harvest(crop)] = chest ? state.storage.harvestCounts.value(id) : state.inventoryCounts.value(id);
    }
    result[StackItem::Fertilizer] = chest ? state.storage.fertilizerCount : state.fertilizerCount;
    return result;
}

bool validSlots(const QVector<InventoryStack> &cells) {
    for (const auto &cell : cells)
        if (!StackItem::valid(cell.item) || cell.quantity < 0 ||
            (cell.item == 0 && cell.quantity != 0) || (StackItem::isTool(cell.item) && cell.quantity != 1)) return false;
    return true;
}

bool distribute(QVector<InventoryStack> &cells, QHash<int,qint64> wanted, bool expand) {
    if (!validSlots(cells)) return false;
    for (auto &cell : cells) {
        if (StackItem::isTool(cell.item)) continue;
        const int keep = int(qMin<qint64>(cell.quantity,wanted.value(cell.item)));
        wanted[cell.item] -= keep;
        cell.quantity=keep;
        if (!keep) cell={};
    }
    QVector<int> order;
    for (CropType crop : CropCatalog::cropTypes()) order.append(StackItem::seed(crop));
    order.append(StackItem::Fertilizer);
    for (CropType crop : CropCatalog::cropTypes()) order.append(StackItem::harvest(crop));
    for (int item : order) {
        if (!wanted.value(item)) continue;
        if (wanted[item] < 0 || wanted[item] > std::numeric_limits<int>::max()) return false;
        auto slot = std::find_if(cells.begin(),cells.end(),[item](const InventoryStack &s){return s.item==item;});
        if (slot == cells.end()) slot=std::find_if(cells.begin(),cells.end(),[](const InventoryStack &s){return !s.item;});
        if (slot == cells.end() && expand && cells.size()<65529) {
            const int old=cells.size(); cells.resize(old+7); slot=cells.begin()+old;
        }
        if (slot == cells.end()) return false;
        if (wanted[item] > std::numeric_limits<int>::max()-slot->quantity) return false;
        slot->item=item; slot->quantity+=int(wanted[item]);
    }
    return true;
}

bool validCropCounts(const QHash<int, int> &counts) {
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        if (CropCatalog::definition(static_cast<CropType>(it.key())).type == CropType::None ||
            it.value() < 0)
            return false;
    }
    return true;
}
}

bool EconomyService::layoutMatches(const EconomyState &state) {
    if (state.bagSlots.size()!=BagSlots || state.chestSlots.size()<7 || state.chestSlots.size()>65536 ||
        !validSlots(state.bagSlots) || !validSlots(state.chestSlots)) return false;
    QHash<int,qint64> tools;
    for (bool chest : {false,true}) {
        auto wanted=totals(state,chest);
        const auto &cells=chest ? state.chestSlots : state.bagSlots;
        for (const auto &cell : cells) {
            if (StackItem::isTool(cell.item)) tools[cell.item]+=cell.quantity;
            else if (cell.item) wanted[cell.item]-=cell.quantity;
            if (cell.item && cell.quantity<=0) return false;
        }
        for (qint64 remaining : wanted) if (remaining) return false;
    }
    return tools.value(StackItem::Hoe)==1 && tools.value(StackItem::Water)==1;
}

bool EconomyService::reconcile(EconomyState &state) {
    if (state.bagSlots.isEmpty() && state.chestSlots.isEmpty()) {
        state.bagSlots.resize(BagSlots); state.chestSlots.resize(42);
        state.bagSlots[0]={StackItem::Hoe,1}; state.bagSlots[1]={StackItem::Water,1};
    }
    if (state.bagSlots.size()!=BagSlots || state.chestSlots.size()<7 || state.chestSlots.size()>65536) return false;
    if (!distribute(state.bagSlots,totals(state,false),false) ||
        !distribute(state.chestSlots,totals(state,true),true)) return false;
    if (state.chestSlots.size()<65529 && std::none_of(state.chestSlots.cbegin(),state.chestSlots.cend(),
        [](const InventoryStack &cell){return !cell.item;})) state.chestSlots.resize(state.chestSlots.size()+7);
    return layoutMatches(state);
}

void EconomyService::changed() {
    auto snapshot=state();
    const bool valid=reconcile(snapshot);
    Q_ASSERT(valid);
    if (valid) { m_bagSlots=snapshot.bagSlots; m_chestSlots=snapshot.chestSlots; ++m_revision; }
}

void EconomyService::consumeSlot(int item) {
    int slot=m_equippedSlot;
    if (slot<0 || slot>=m_bagSlots.size() || m_bagSlots[slot].item!=item) {
        slot=-1;
        for (int i=0; i<m_bagSlots.size(); ++i) if (m_bagSlots[i].item==item) {slot=i; break;}
    }
    if (slot>=0 && --m_bagSlots[slot].quantity==0) m_bagSlots[slot]={};
}

bool EconomyService::moveStack(const StackMove &move) {
    if (m_applyingItem || move.revision!=m_revision || move.quantity<=0) return false;
    auto candidate=state();
    auto &source=move.sourceChest ? candidate.chestSlots : candidate.bagSlots;
    auto &target=move.targetChest ? candidate.chestSlots : candidate.bagSlots;
    if (move.source<0 || move.source>=source.size() || move.target<0 || move.target>=target.size() ||
        (move.sourceChest==move.targetChest && move.source==move.target)) return false;
    auto &from=source[move.source]; auto &to=target[move.target];
    if (!(from==move.expected) || !from.item || move.quantity>from.quantity) return false;
    if (!to.item || to.item==from.item) {
        if (move.quantity>std::numeric_limits<int>::max()-to.quantity || (StackItem::isTool(from.item) && to.item)) return false;
        to.item=from.item; to.quantity+=move.quantity; from.quantity-=move.quantity;
        if (!from.quantity) from={};
    } else {
        if (move.quantity!=from.quantity) return false;
        std::swap(from,to);
    }
    candidate.inventoryCounts.clear(); candidate.seedCounts.clear(); candidate.fertilizerCount=0;
    candidate.storage={};
    for (bool chest : {false,true}) for (const auto &stack : chest ? candidate.chestSlots : candidate.bagSlots) {
        if (!stack.item || StackItem::isTool(stack.item)) continue;
        const int id=static_cast<int>(StackItem::crop(stack.item));
        if (stack.item==StackItem::Fertilizer) {
            int &value=chest ? candidate.storage.fertilizerCount : candidate.fertilizerCount;
            if (stack.quantity>std::numeric_limits<int>::max()-value) return false;
            value+=stack.quantity;
        } else {
            auto &counts=chest ? (stack.item/1000==2 ? candidate.storage.seedCounts : candidate.storage.harvestCounts)
                              : (stack.item/1000==2 ? candidate.seedCounts : candidate.inventoryCounts);
            if (stack.quantity>std::numeric_limits<int>::max()-counts.value(id)) return false;
            counts[id]+=stack.quantity;
        }
    }
    return restore(candidate);
}

EconomyService::EconomyService(int inventoryCapacity)
    : m_inventory(inventoryCapacity) {
    m_seedCounts.insert(static_cast<int>(CropType::Wheat),
                        GameConstants::InitialWheatSeeds);
    m_seedCounts.insert(static_cast<int>(CropType::Potato),
                        GameConstants::InitialPotatoSeeds);
    changed();
}

int EconomyService::seedCount(CropType type) const {
    return m_seedCounts.value(static_cast<int>(type), 0);
}

bool EconomyService::collectHarvest(CropType type, int quantity) {
    if (m_applyingItem || !m_inventory.canAdd(type,quantity)) return false;
    auto candidate=state(); candidate.inventoryCounts[static_cast<int>(type)]+=quantity;
    if (!reconcile(candidate) || !m_inventory.add(type,quantity)) return false;
    changed(); return true;
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
    consumeSlot(StackItem::seed(type)); changed();
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
    consumeSlot(StackItem::Fertilizer); changed();
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

    auto candidate=state();
    if (type==CropType::None) candidate.fertilizerCount+=quantity;
    else candidate.seedCounts[static_cast<int>(type)]+=quantity;
    if (!reconcile(candidate)) { result.failure=SeedPurchaseFailure::InvalidRequest; return result; }
    m_coins -= static_cast<int>(totalCost);
    if (type == CropType::None) {
        m_fertilizerCount += quantity;
    } else {
        m_seedCounts[static_cast<int>(type)] += quantity;
    }
    result.success = true;
    result.coinsSpent = static_cast<int>(totalCost);
    changed();
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
    changed();
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
    changed();
    return {true, itemsSold, static_cast<int>(revenue)};
}

EconomyState EconomyService::state() const {
    return {m_coins, m_inventory.capacity(), m_inventory.counts(),
            m_seedCounts, m_fertilizerCount, m_storage, m_bagSlots, m_chestSlots};
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
    auto candidate=state;
    if (!reconcile(candidate)) return false;
    m_coins = state.coins;
    m_fertilizerCount = state.fertilizerCount;
    m_inventory = restoredInventory;
    m_seedCounts = state.seedCounts;
    m_storage = state.storage;
    m_bagSlots=candidate.bagSlots; m_chestSlots=candidate.chestSlots; ++m_revision;
    for (CropType type : CropCatalog::cropTypes()) {
        if (!m_seedCounts.contains(static_cast<int>(type))) {
            m_seedCounts.insert(static_cast<int>(type), 0);
        }
    }
    return true;
}
